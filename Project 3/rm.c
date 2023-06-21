/**
 * CS342 Spring 2023 - Project 3
 * This files contains the implementation of the rm interface.
 * @author Selahattin Cem Öztürk 
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "rm.h"

typedef struct{
    int numOfThreads; //number of threads simulated
    int numOfResources; //number of resources
    int available[MAXR]; //all available resources in the system
    int max_demands[MAXP][MAXR]; //maximum demans of the processes
    int requests[MAXP][MAXR];
    int allocations[MAXP][MAXR]; //allocation of each process so far
    int needs[MAXP][MAXR]; //max_demands - allocations
}Banker;

// global variables
int DA;  // indicates if deadlocks will be avoided or not
int N;   // number of processes
int M;   // number of resource types
int ExistingRes[MAXR]; // Existing resources vector
Banker banker; //for avoidance algorithm
pthread_mutex_t lock;
pthread_cond_t cond;
pthread_t threadArray[MAXP];
int running[MAXP];
//helper functions
int canAllocate(int request[]);
int canAllocate2(int available[],int request[]);
void allocate(int id,int request[]);
void releaseResource(int id,int release[]);
int is_safe_avoidance(int tid, int demand[]);
void print_vector(int size, int vector[]);
void print_matrix(int num_rows, int num_cols, int matrix[MAXP][MAXR]);

/**
 * if all the resource types are sufficient enough, returns 0 else returns 1 
 * Example if a process wants 5 printers while only 4 printer available maximum,  returns 1
 * @param request count array of the requested resources
 */
int canAllocate(int request[]){
    for(int i = 0; i < M; i++){
        if(banker.available[i] < request[i]){
            return 1;
        }
    }
    return 0;
}


/**
 * allocate the requested resource to thread that has the tid that is equals to id
 * @param id thread id 
 * @param request count array of the requested resources
 */
void allocate(int id,int request[]){
    for(int i = 0; i < M; i++){
        banker.available[i] = banker.available[i] - request[i];
        banker.needs[id][i] = banker.needs[id][i] - request[i];
        banker.allocations[id][i] += request[i];
        banker.requests[id][i] = 0;
    }
    
}
/**
 * release the resources used by the thread that has the tid that is equals to id
 * @param id thread id 
 * @param release count array of the released resources
 */
void releaseResource(int id,int release[]){
    for(int i = 0; i < M; i++){
        banker.available[i] = banker.available[i] + release[i];
        banker.allocations[id][i] -= release[i];
    }
}

/**
 * starts the thread, and set it's running array value to 1.
 * @param tid thread id 
 */
int rm_thread_started(int tid)
{
    pthread_mutex_lock(&lock);
    if(tid< 0 || tid >= N){
        printf("Error: invalid tid. Exiting.\n");
        return -1;
    }
    threadArray[tid] = pthread_self();
    running[tid] = 1;
    pthread_mutex_unlock(&lock);
    return 0;
}

/**
 * ends the thread, and set it's running array value to 0.
 */
int rm_thread_ended()
{
    int ret = -1;
    pthread_mutex_lock(&lock);
    pthread_t check = pthread_self();
    int index =-1;
    for(int i = 0; i < N; i++){
        if(threadArray[i] == check){
            index = i;
            running[i] = 0;
            ret = 0;
            break;
        }
    }
    for(int i = 0; i < M; i++){
        banker.needs[index][i] = 0;
        banker.allocations[index][i] = 0;
    }
    pthread_mutex_unlock(&lock);
    return (ret);
}

/**
 * thread claims the resources that is going to use in it's life time, relative max_demands matrix setted to claim array's value.
 * @param claim claimed array 
 */
int rm_claim (int claim[])
{
    int ret = 0;
    pthread_t check = pthread_self();
    int index =-1;
    pthread_mutex_lock(&lock);
    for(int i = 0; i < N; i++){
        if(threadArray[i] == check && running[i] == 1){
            index = i;
            break;
        }
    }
    if(DA == 1){
        if(sizeof(*claim) /sizeof(int) >M)
            return -1;
        for(int i = 0; i < M; i++){
            banker.max_demands[index][i] = claim[i];
            banker.needs[index][i] = claim[i];
        }
    }else{
        if(sizeof(*claim) /sizeof(int) >M)
            return -1;
        for(int i = 0; i < M; i++){
            banker.max_demands[index][i] = claim[i];
            banker.needs[index][i] = claim[i];
        }
    }
    pthread_mutex_unlock(&lock);
    return(ret);
}

/**
 * initialize the library's arrays, and the avoid protocol. 
 * @param p_count process count
 * @param r_count resource count 
 * @param r_exist existing resource array 
 * @param avoid avoidence method, 1 if deadlock avoidence used.
 */
int rm_init(int p_count, int r_count, int r_exist[],  int avoid)
{
    if (p_count > MAXP) {
        printf("Error: Max number of processes exceeded. Exiting\n");
        return -1;
    }
    if (r_count > MAXR) {
        printf("Error: Max number of resource types exceeded. Exiting\n");
        return -1;
    }
    DA = avoid;
    N = p_count;
    M = r_count;
    banker.numOfResources = M;
    banker.numOfThreads = N;
    for(int i = 0; i < M; i++){
        ExistingRes[i] = r_exist[i];
        banker.available[i] = r_exist[i];
    }
    for(int i = 0; i < N; i++){
        running[i] = 0;
    }
    for(int i = 0; i < N;i++){
        for(int j = 0; j < M;j++){
            banker.requests[i][j] = 0;
            banker.allocations[i][j] = 0;
            banker.max_demands[i][j] = 0;
            banker.needs[i][j] = 0;
        }
    }
    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("Error: Mutex lock initialization failed.\n");
        return -1;
    }
    if (pthread_cond_init(&cond, NULL) != 0) {
        printf("Error: Condition variable initialization failed.\n");
        return -1;
    }
    return 0;
    
}

/**
 * same with the canAllocate but this time, it will compare the request with any arbitrary available, instead of 
 * comparing with banker's avaiilable.
 * @param available arbitrary array thet will be used to compare whether it has sufficient resource count
 * @param request count array of the requested resources
 */
int canAllocate2(int available[],int request[]){
    for(int i = 0; i < M; i++){
        if(available[i] < request[i]){
            return 0;
        }
    }
    return 1;
}

/**
 * check whether it is still in the safe state if the demand is given to process. Safe state check for banker algorithm in other words.
 * @param tid thread id
 * @param demand demand array 
 */
int is_safe_avoidance(int tid, int demand[]) {
    int ret = 0;
    int copyAvailable[M];
    int copyRunning[N];
    int copyAllocations[N][M];
    int copyNeeds[N][M];
    
    //fill the copies for checking the safe state
    for(int i = 0; i < M; i++){
        copyAvailable[i] = banker.available[i] ;
    }
    for(int i = 0; i < N; i++){
        copyRunning[i] =running[i] ;
    }
    for(int i = 0; i < N;i++){
        for(int j = 0; j < M;j++){
            copyAllocations[i][j] = banker.allocations[i][j];
            copyNeeds[i][j] = banker.needs[i][j];
        }
    }
    //simulate that it is allocaated 
    for(int j = 0; j < M;j++){
        copyAllocations[tid][j] += demand[j]; 
        copyNeeds[tid][j] -= demand[j];
        copyAvailable[j] -= demand[j];
    }
    //chech if it is still in the safe state 
    for(int i = 0; i < N; i++){
        if( copyRunning[i] == 1 && canAllocate2( copyAvailable,copyNeeds[i]) )
        {
            for(int j = 0; j < M; j++){
                copyAvailable[j] += copyAllocations[i][j];
            }
            copyRunning[i] = 0;
            i = -1;
        }
    }
    for(int i = 0; i < N; i++){
        if(copyRunning[i] == 1)
        {
            ret++;
        }   
    }
    return (ret);
}

/**
 * thread request some resources, it will be given if its appropiate according to avoid protocol.
 * @param request request array 
 */
int rm_request (int request[])
{
    int ret = 0;
    pthread_t check = pthread_self();
    int index =-1;
    if(sizeof(*request) / sizeof(int) > M){
        printf("Request more than the number of resource type. Exiting...\n");
        return -1;
    }
    for(int i = 0; i < M; i++){
        if(ExistingRes[i] < request[i]){
            printf("More request than the total instance for one or more resource. Exiting... \n");
            return -1;
        }
    }
    pthread_mutex_lock(&lock);
    for(int i = 0; i < N; i++){
        if(threadArray[i] == check && running[i] == 1){
            index = i;
            break;
        }
    }
    for(int i = 0; i < M; i++)
        banker.requests[index][i] = request[i];
    if(DA == 0){
        while(canAllocate(request)){
            pthread_cond_wait(&cond,&lock);
        } 
        allocate(index,request);
    }else{//AVOIDENCE
        while(is_safe_avoidance(index,request)){
            pthread_cond_wait(&cond,&lock);
        }
        allocate(index,request);
    }

    pthread_mutex_unlock(&lock);
    return(ret);
}

/**
 * thread release some resources 
 * @param release release array 
 */
int rm_release (int release[]){
    int ret = 0;
    pthread_t check = pthread_self();
    int index =-1;
    if(sizeof(*release) / sizeof(int) > M){
        printf("Release more than the number of resource type. Exiting...\n");
        return -1;
    }
    pthread_mutex_lock(&lock);
    for(int i = 0; i < N; i++){
        if(threadArray[i] == check && running[i] == 1){
            index = i;
            break;
        }
    }
    for(int i = 0; i < M; i++){
        if(banker.allocations[index][i] < release[i]){
            printf("More release than the total allocation. Exiting... \n");
            return -1;
        }
    }
    
    releaseResource(index,release);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&lock);
    return (ret);
}

/**
 * check if there is a deadlocked process, returns the number of processes deadlocked.
 */
int rm_detection() //Deadlock detection part (checking the safe state part of banker algorithm)
{
    if(DA == 1){
        return 0;
    }
    int ret = 0;
    //need to make copies of the banker's vectors and matrices
    int copyAvailable[M];
    int copyRunning[N];
    int copyAllocations[N][M];
    int copyNeeds[N][M];
    pthread_mutex_lock(&lock);
    for(int i = 0; i < M; i++){
        copyAvailable[i] =banker.available[i] ;
    }
    for(int i = 0; i < N; i++){
        copyRunning[i] =running[i] ;
    }
    for(int i = 0; i < N;i++){
        for(int j = 0; j < M;j++){
            copyAllocations[i][j] = banker.allocations[i][j];
            copyNeeds[i][j] = banker.needs[i][j];
        }
    }
    for(int i = 0; i < N; i++){
        if(copyRunning[i] == 1 && canAllocate2(copyAvailable,copyNeeds[i]) )
        {
            for(int j = 0; j < M; j++){
                copyAvailable[j] += copyAllocations[i][j];
            }
            copyRunning[i] = 0;
            i = -1;
        }
        
    }
    for(int i = 0; i < N; i++){
        if(copyRunning[i] == 1)
        {
            ret++;
        }   
    }
    pthread_mutex_unlock(&lock);
    return (ret);
}
/**
 * prints the taken vector
 * @param size size of the vector
 * @param vector printed vector
 */
void print_vector(int size, int vector[]) {
    printf("\t");
    for (int i = 0; i < size; i++) {
        printf("R%d\t", i);
    }
    printf("\n\t");
    for (int i = 0; i < size; i++) {
        printf("%d \t", vector[i]);
    }
    printf("\n");
}
/**
 * prints the taken matrix
 * @param row row of the matrix
 * @param column column of the matrix
 * @param matrix printed matrix
 */
void print_matrix(int num_rows, int num_cols, int matrix[MAXP][MAXR]) {
    printf("\t");
    for (int i = 0; i < num_cols; i++) {
        printf("R%d\t", i);
    }
    printf("\n");
    for (int i = 0; i < num_rows; i++) {
        printf("T%d:\t", i);
        for (int j = 0; j < num_cols; j++) {
            printf("%d \t", matrix[i][j]);
        }
        printf("\n");
    }
}
/**
 * prints the zero matrix according to N and M of the banker.
 */
void printAllZeros() {
    printf("\t");
    for (int i = 0; i < M; i++) {
        printf("R%d\t", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d:\t", i);
        for (int j = 0; j < M; j++) {
            printf("0 \t");
        }
        printf("\n");
    }
}
/**
 * prints the current state 
 * @param hmsg printed at top
 */
void rm_print_state (char hmsg[])
{
    pthread_mutex_lock(&lock);
    printf("\n\n");
    printf("##############################\n%s\n##############################",hmsg);
    char *policy ;
    if(DA == 0){
        policy ="No Deadlock Avoidence";
    }else if(DA == 1){
        policy =  "Deadlock Avoidence";
    }
    printf("\nPolicy = %s\n", policy);
    printf("\nN = %d\nM = %d\n", N, M);
    printf("\nExist:\n");
    print_vector(M, ExistingRes);
    printf("\nAvailable: \n");
    print_vector(M, banker.available);
    printf("\nAllocation: \n");
    print_matrix(N,M, banker.allocations);
    printf("\nRequest: \n");
    print_matrix(N,M, banker.requests);
    if(DA == 1){
        if (banker.max_demands != NULL) {
            printf("\nMaxDemand:\n");
            print_matrix(N, M, banker.max_demands);
        }
        if (banker.needs != NULL) {
            printf("\nNeed: \n");
            print_matrix(N, M, banker.needs);
        }
    }else{
        printf("\nMaxDemand:\n");
        printAllZeros();
        printf("\nNeed:\n");
        printAllZeros();
    }
    printf("\n\n");
    pthread_mutex_unlock(&lock);
    
}
