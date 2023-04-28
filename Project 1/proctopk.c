#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h> 
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "shareddefs.h"
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define MEM_NAME "/sharedmem"
int N;
int SIZE;
int topK;
struct Node { 
    char * word; 
    struct Node* next; 
    int count;
};
void printList(struct Node *heads) { 
    struct Node *temp = heads; 
    while(temp != NULL){ 
	printf("element is %s count is %d \n",temp->word,temp->count);
        temp = temp->next; 
    } 
} 
void push(struct Node** head_ref, char * new_word,int count) {
    struct Node* temp = (*head_ref);
    if(temp != NULL){
        while (  temp !=NULL && (strcmp(temp->word,new_word) !=0) ){
            temp = temp->next;
            if(temp == (*head_ref)){
                break;
            }
        }
        if( temp != NULL && (strcmp(temp->word,new_word) ==0)){
            temp->count = temp->count + count;
        }else{
            struct Node* new_node = (struct Node*)malloc(sizeof(struct Node)); 
            new_node->word  = new_word; 
            new_node->count = count;
            new_node->next = (*head_ref); 
            (*head_ref) = new_node; 
        }
    }else{
        struct Node* new_node = (struct Node*)malloc(sizeof(struct Node)); 
            new_node->word  = new_word; 
            new_node->count = count;
            new_node->next = (*head_ref); 
            (*head_ref) = new_node; 
    }
}
void swapNodes(struct Node* a, struct Node* b) {
    char* temp = a->word;
    int count = a->count;

    a->word = b->word;
    a->count = b->count;

    b->word = temp;
    b->count = count;
}
void sortNodes(struct Node** head) {
    int swapped, i;
    struct Node* ptr1;
    struct Node* lptr = NULL;

    // Check for empty list
    if (*head == NULL)
        return;

    do {
        swapped = 0;
        ptr1 = *head;

        while (ptr1->next != lptr) {
            if (ptr1->count < ptr1->next->count) {
                swapNodes(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    } while (swapped);
}
void toUpperCase(char** str) {
    int len = strlen(*str);

    for (int i = 0; i < len; i++) {
        (*str)[i] = toupper((*str)[i]);
    }
}

void findFreq(char* fileName,int i){
    
    struct item item; 
    int n;
    //pid_t n1 = fork();

    int shm_fd;
 
    /* pointer to shared memory object */
    void* ptr;
 
    /* open the shared memory object */
    shm_fd = shm_open(MEM_NAME, O_RDWR, 0666);
 
    /* memory map the shared memory object */
    ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, i*4096);

    
	FILE *fp;
	char line[128];
	struct Node *headchild = NULL;
    fp = fopen(fileName, "r");
    char* word;
    while (fscanf(fp, " %1023s", line) == 1){
        word = strdup(line);
        toUpperCase(&word);
        push(&headchild,word,1);
    }
    fclose(fp);
    struct Node* temp = NULL;
    char text[64];
    char wordd[64]; 
    sortNodes(&headchild);
    int topKCounter = 0;
    for(temp = headchild; temp!= NULL&&topKCounter < topK; temp = temp->next){
        
        sprintf(text, "%d", temp->count);

        sprintf(ptr, "%s %s ", temp->word,text);
        char countt[64];
        char wordd[64];
        sscanf(ptr,"%s %s",&wordd,&countt);

        ptr += strlen(temp->word);
        ptr += strlen("  ");
        ptr += strlen(text);
        topKCounter++;
        
    }
    
}
void writer(char * fileName,char * word,int count){

   FILE *fp;
   fp = fopen(fileName, "a+");
   fprintf(fp,"%s %d \n",word,count);
   fclose(fp);

}
int main(int argc, char* argv[]){

    clock_t start_time = clock();
    pid_t n;
    struct Node* head = NULL;
    if(argc < 5){
        printf("argument count can not be < 5");
        return 1;
    }else if (argc > 14){
        printf("argument count can not be > 14");
        return 1;
    }
    N = atoi(argv[3]);
    topK = atoi(argv[1]);
    SIZE = N*4096;
    int shm_fd;
 
    /* pointer to shared memory object */
    void* ptr;
 
    /* create the shared memory object */
    shm_fd = shm_open(MEM_NAME, O_CREAT | O_RDWR, 0666);
 
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE);
 
    /* memory map the shared memory object */
    ptr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    void* tempptr = ptr;

    for(int i = 0; i < atoi(argv[3]);i++){
        n = fork();
        if(n == 0){
            findFreq(argv[i + 4],i);
            exit(0);
        }else{
            
            wait(NULL);
        }
    }
    
    
    FILE *fp;
    fp = fopen(argv[2], "a+");
    
    for(int j = 0; j < N;j++){
        char* text; 
        char countt[64];
        char wordd[64];
        char* word;
        for(int i = 0; i < topK; i++){
            sscanf(ptr,"%s %s ",&wordd,&countt);
            word = strdup(wordd);
            text = strdup(countt);
            push(&head,word,atoi(countt));

            ptr += strlen(word);
            ptr += strlen("  ");
            ptr += strlen(countt);
        }

        ptr = tempptr;
        ptr += 4096*(j+1);
    }

    sortNodes(&head);
    struct Node *tempHead = head; 
    int topKCounter = 0;
    while(tempHead != NULL && topKCounter < topK) { 
	    writer(argv[2],tempHead->word,tempHead->count);	
	    tempHead = tempHead->next; 
        topKCounter++;
	}  
    fclose(fp);
    shm_unlink(MEM_NAME);
    clock_t end_time = clock();

    printf("The program took: %zd milliseconds.\n", end_time - start_time);
}
