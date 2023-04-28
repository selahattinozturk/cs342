#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <mqueue.h>
#include <errno.h>
#include <unistd.h>
#include <mqueue.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <getopt.h>
#include <math.h>
#include "mps_utils.h"
#define USEC_VAL 1000
int opt;
int n = 2;                 // default value for -n
char *sap = "M";           // default value for -a
char *qs = "RM";           // default value for -a
char *alg = "RR";          // default value for -s
int q = 20;                // default value for -s
char *infile = "in.txt";   // default value for -i
char *outfile = "out.txt"; // default value for -o
int mode = 1;              // default value for -m
int t = 200;
int t1 = 10;
int t2 = 1000;
int l = 100;
int l1 = 10;
int l2 = 500;
int PC = 10; // variables for -r option
int rFlag = 0;
int iFlag = 0;
int oFlag = 0;
pthread_mutex_t *lock;
pthread_mutex_t writelock;
struct burst_node **queues;
struct burst_node *finishedBursts;
struct timeval tval_start, tval_finish, tval_result;
FILE *fp ;
double r2(){
    return (double)rand() / (double)RAND_MAX;
}
int time_diff(struct timeval start_time, struct timeval finish_time){
    double start_ms = (double)start_time.tv_sec * 1000.0 + (double)start_time.tv_usec / 1000.0;
    double finish_ms = (double)finish_time.tv_sec * 1000.0 + (double)finish_time.tv_usec / 1000.0;
    return ((int)(finish_ms - start_ms));
}
struct burst_node *sort_list(struct burst_node *head){
    if (head == NULL || head->next == NULL){
        return head;
    }
    struct burst_node *curr, *sorted_head, *sorted_tail;
    sorted_head = sorted_tail = head;
    curr = head->next;
    sorted_tail->next = NULL;
    while (curr != NULL){
        struct burst_node *prev = NULL;
        struct burst_node *node = sorted_head;
        while (node != NULL && node->process_id <= curr->process_id){
            prev = node;
            node = node->next;
        }
        struct burst_node *next = curr->next;
        if (prev == NULL){
            sorted_head = curr;
        }
        else{
            prev->next = curr;
        }
        curr->next = node;
        if (curr->next == NULL){
            sorted_tail = curr;
        }
        curr = next;
    }
    return sorted_head;
}
void *process_simulator(void *p){
    struct t_param *param = (struct t_param *)p;
    int queueIndex = 0;
    if (param->numberOfQueues == 1){
        queueIndex = 0;
    }
    else{
        queueIndex = param->processor_id;
    }
    while (1){
        if (strcmp(alg, "FCFS") == 0){
            pthread_mutex_lock(&lock[queueIndex]);
            // select from the queue
            if (queues[queueIndex]->next == NULL){ // no dummy item but empty queue
                pthread_mutex_unlock(&lock[queueIndex]);
                // printf("no dummy %d\n" ,param->processor_id + 1);
                usleep(USEC_VAL);
            }
            else if (queues[queueIndex]->next->dummy_flag == 1 && queues[queueIndex]->burst_length == 0){ // there is dummy item in the queue
                pthread_mutex_unlock(&lock[queueIndex]);
                pthread_exit(NULL);
            }
            else{ // process
                struct burst_node *temp = (struct burst_node *)malloc(sizeof(struct burst_node));
                struct burst_node *temp2 = (struct burst_node *)malloc(sizeof(struct burst_node));
                temp = queues[queueIndex];
                temp2 = queues[queueIndex]->next;
                while (temp->next->next != NULL){
                    temp2 = temp2->next;
                    temp = temp->next;
                } // temp points to the head of the queue.
                if (temp2->dummy_flag == 1){
                    pthread_mutex_unlock(&lock[queueIndex]);
                    pthread_exit(NULL);
                }
                temp->next = NULL; // removed from the queue can release the lock.
                pthread_mutex_unlock(&lock[queueIndex]);
                gettimeofday(&tval_finish, NULL);
                if (mode == 2){
                    if (oFlag == 0){
                        printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                    }
                    else if (oFlag == 1){ 
                        pthread_mutex_lock(&writelock);
                        // write to the text file
                        fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        // close the file
                        pthread_mutex_unlock(&writelock);
                    }
                }
                if (mode == 3){
                    if (oFlag == 0){
                        printf("Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                        printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                    }
                    else if (oFlag == 1){ 
                        pthread_mutex_lock(&writelock);
                        
                        // write to the text file
                        fprintf(fp, "Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                        fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        // close the file
                           pthread_mutex_unlock(&writelock);
                    }
                }
                usleep(USEC_VAL * temp2->burst_length);
                if (mode == 3){
                    if (oFlag == 0){
                        printf("Burst that has the following information is finished by CPU %d:\n", param->processor_id + 1);
                        printf("time = %d, cpu= %d, pid= %d, burstlen=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length);
                    }
                    else if (oFlag == 1){
                        pthread_mutex_lock(&writelock);
                        
                        // write to the text file
                        fprintf(fp, "Burst that has the following information is finished by CPU %d:\n", param->processor_id + 1);
                        fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length);
                        // close the file
                           pthread_mutex_unlock(&writelock);
                    }
                }
                gettimeofday(&tval_finish, NULL);
                temp2->finish_time = time_diff(tval_start, tval_finish);
                pthread_mutex_lock(&lock[queueIndex]);
                queues[queueIndex]->burst_length = queues[queueIndex]->burst_length - temp2->burst_length;
                temp2->next = finishedBursts->next;
                temp2->processor_id = param->processor_id + 1;
                finishedBursts->next = temp2;
                finishedBursts->next->remaining_time = 0;
                pthread_mutex_unlock(&lock[queueIndex]);
            }
        }
        else if (strcmp(alg, "SJF") == 0){
            pthread_mutex_lock(&lock[queueIndex]);
            // select from the queue
            if (queues[queueIndex]->next == NULL){ // no dummy item but empty queue
                pthread_mutex_unlock(&lock[queueIndex]);
                usleep(USEC_VAL);
            }
            else if (queues[queueIndex]->next->dummy_flag == 1 && queues[queueIndex]->burst_length == 0){ // there is dummy item in the queue
                pthread_mutex_unlock(&lock[queueIndex]);
                pthread_exit(NULL);
            }
            else{ // process
                struct burst_node *temp = (struct burst_node *)malloc(sizeof(struct burst_node));
                struct burst_node *tempBefore = (struct burst_node *)malloc(sizeof(struct burst_node));
                struct burst_node *test = (struct burst_node *)malloc(sizeof(struct burst_node));
                struct burst_node *testBefore = (struct burst_node *)malloc(sizeof(struct burst_node));
                temp = queues[queueIndex];
                tempBefore = queues[queueIndex];
                test = queues[queueIndex];
                testBefore = queues[queueIndex];
                while (test->next != NULL){
                    test = test->next;
                    if (test->burst_length <= temp->burst_length && test->dummy_flag == 0){
                        temp = test;
                        tempBefore = testBefore;
                    }
                    testBefore = testBefore->next;
                } // temp points to the head of the sj.
                if (temp->remaining_time == 0){
                    pthread_mutex_unlock(&lock[queueIndex]);
                    pthread_exit(NULL);
                }
                tempBefore->next = temp->next;
                temp->next = NULL;
                pthread_mutex_unlock(&lock[queueIndex]);
                gettimeofday(&tval_finish, NULL);
                if (mode == 2){
                    if (oFlag == 0)
                        printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp->process_id, temp->burst_length, temp->remaining_time);
                    if (oFlag == 1){ 
                        pthread_mutex_lock(&writelock);
                        
                        // write to the text file
                        fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp->process_id, temp->burst_length, temp->remaining_time);
                        // close the file
                        pthread_mutex_unlock(&writelock);
                    }
                }
                if (mode == 3){
                    if (oFlag == 0){
                        printf("Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                        printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp->process_id, temp->burst_length, temp->remaining_time);
                    }
                    if (oFlag == 1){ 
                        pthread_mutex_lock(&writelock);
                        
                        // write to the text file
                        fprintf(fp, "Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                        fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp->process_id, temp->burst_length, temp->remaining_time);
                        // close the file
                        pthread_mutex_unlock(&writelock);
                    }
                }
                usleep(USEC_VAL * temp->burst_length);
                if (mode == 3){
                    if (oFlag == 0){
                        printf("Burst that has the following information is finished by CPU %d:\n", param->processor_id + 1);
                        printf("time = %d, cpu= %d, pid= %d, burstlen=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp->process_id, temp->burst_length);
                    }
                    if (oFlag == 1){ 
                        pthread_mutex_lock(&writelock);
                        
                        // write to the text file
                        fprintf(fp, "Burst that has the following information is finished by CPU %d:\n", param->processor_id + 1);
                        fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp->process_id, temp->burst_length);
                        // close the file
                           pthread_mutex_unlock(&writelock);
                    }
                }
                gettimeofday(&tval_finish, NULL);
                temp->finish_time = time_diff(tval_start, tval_finish);
                pthread_mutex_lock(&lock[queueIndex]);
                queues[queueIndex]->burst_length = queues[queueIndex]->burst_length - temp->burst_length;
                temp->next = finishedBursts->next;
                temp->processor_id = param->processor_id + 1;
                finishedBursts->next = temp;
                pthread_mutex_unlock(&lock[queueIndex]);
            }
        }
        else if (strcmp(alg, "RR") == 0){
            pthread_mutex_lock(&lock[queueIndex]);
            // select from the queue
            if (queues[queueIndex]->next == NULL){ // no dummy item but empty queue
                pthread_mutex_unlock(&lock[queueIndex]);
                usleep(USEC_VAL);
            }
            else if (queues[queueIndex]->next->dummy_flag == 1 && queues[queueIndex]->burst_length == 0){ // there is dummy item in the queue
                pthread_mutex_unlock(&lock[queueIndex]);
                pthread_exit(NULL);
            }
            else{ // process
                struct burst_node *temp = (struct burst_node *)malloc(sizeof(struct burst_node));
                struct burst_node *temp2 = (struct burst_node *)malloc(sizeof(struct burst_node));
                temp = queues[queueIndex];
                temp2 = queues[queueIndex]->next;
                while (temp->next->next != NULL){
                    temp2 = temp2->next;
                    temp = temp->next;
                } // temp->next points to the head of the queue.
                if (temp2->dummy_flag == 1){
                    pthread_mutex_unlock(&lock[queueIndex]);
                    pthread_exit(NULL);
                }
                temp->next = NULL;
                pthread_mutex_unlock(&lock[queueIndex]);
                temp2->processor_id = param->processor_id + 1;
                if (temp2->remaining_time > q){
                    gettimeofday(&tval_finish, NULL);
                    if (mode == 2){
                        if (oFlag == 0)
                            printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                             
                            // write to the text file
                            fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                            // close the file
                               pthread_mutex_unlock(&writelock);
                        }
                    }
                    if (mode == 3){
                        if (oFlag == 0){
                            printf("Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                            printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        }
                        if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                             
                            // write to the text file
                            fprintf(fp, "Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                            fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                            // close the file
                               pthread_mutex_unlock(&writelock);
                        }
                    }
                    usleep(USEC_VAL * q);
                    temp2->remaining_time = temp2->remaining_time - q;
                    pthread_mutex_lock(&lock[queueIndex]);
                    queues[queueIndex]->burst_length = queues[queueIndex]->burst_length - q;
                    if (queues[queueIndex]->next != NULL){
                        if (queues[queueIndex]->next->dummy_flag == 1){
                            if (queues[queueIndex]->next->next != NULL){
                                temp2->next = queues[queueIndex]->next->next;
                                queues[queueIndex]->next->next = temp2;
                            }
                            else{
                                queues[queueIndex]->next->next = temp2;
                            }
                        }
                        else if (queues[queueIndex]->next->dummy_flag == 0){
                            temp2->next = queues[queueIndex]->next;
                            queues[queueIndex]->next = temp2;
                        }
                    }
                    else{
                        queues[queueIndex]->next = temp2;
                    }
                    if (mode == 3){
                        if (oFlag == 0){
                            printf("Burst that has the following information processed and putted back to queue of CPU %d:\n", param->processor_id + 1);
                            printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        }
                        if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                             
                            // write to the text file
                            fprintf(fp, "Burst that has the following information processed and putted back to queue of CPU %d:\n", param->processor_id + 1);
                            fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                            // close the file
                               pthread_mutex_unlock(&writelock);
                        }
                    }
                    pthread_mutex_unlock(&lock[queueIndex]);
                }
                else if (temp2->remaining_time == q){
                    gettimeofday(&tval_finish, NULL);
                    if (mode == 2){
                        if (oFlag == 0)
                            printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                             
                            // write to the text file
                            fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                               pthread_mutex_unlock(&writelock);
                        }
                    }
                    if (mode == 3){
                        if (oFlag == 0){
                            printf("Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                            printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        }
                        if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                             
                            // write to the text file
                            fprintf(fp, "Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                            fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                               pthread_mutex_unlock(&writelock);
                        }
                    }
                    usleep(USEC_VAL * q);
                    if (mode == 3){
                        if (oFlag == 0){
                            printf("Burst that has the following information is finished by CPU %d:\n", param->processor_id + 1);
                            printf("time = %d, cpu= %d, pid= %d, burstlen=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length);
                        }
                        if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                             
                            // write to the text file
                            fprintf(fp, "Burst that has the following information is finished by CPU %d:\n", param->processor_id + 1);
                            fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length);
                               pthread_mutex_unlock(&writelock);
                        }
                    }
                    pthread_mutex_lock(&lock[queueIndex]);
                    queues[queueIndex]->burst_length = queues[queueIndex]->burst_length - q;
                    temp2->remaining_time = 0;
                    gettimeofday(&tval_finish, NULL);
                    temp2->finish_time = time_diff(tval_start, tval_finish);
                    temp2->next = finishedBursts->next;
                    finishedBursts->next = temp2;
                    pthread_mutex_unlock(&lock[queueIndex]);
                }
                else if (temp2->remaining_time < q){
                    gettimeofday(&tval_finish, NULL);
                    if (mode == 2){
                        if (oFlag == 0)
                            printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish),
                                   param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                             
                            // write to the text file
                            fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n", time_diff(tval_start, tval_finish),
                                    param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                               pthread_mutex_unlock(&writelock);
                        }
                    }
                    if (mode == 3){
                        if (oFlag == 0){
                            printf("Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                            printf("time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        }
                        if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                             
                            // write to the text file
                            fprintf(fp, "Burst that has the following information picked to processed by CPU %d:\n", param->processor_id + 1);
                            fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d, remainingtime=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length, temp2->remaining_time);
                        
                           pthread_mutex_unlock(&writelock);
                        }
                    }
                    usleep(USEC_VAL * (temp2->remaining_time));
                    if (mode == 3){
                        if (oFlag == 0){
                        printf("Burst that has the following information is finished by CPU %d:\n", param->processor_id + 1);
                        printf("time = %d, cpu= %d, pid= %d, burstlen=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length);
                        }
                        if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                        
                        // write to the text file
                        fprintf(fp, "Burst that has the following information is finished by CPU %d:\n", param->processor_id + 1);
                        fprintf(fp, "time = %d, cpu= %d, pid= %d, burstlen=%d\n\n", time_diff(tval_start, tval_finish), param->processor_id + 1, temp2->process_id, temp2->burst_length);
                        
                           pthread_mutex_unlock(&writelock);
                        }
                    }
                    pthread_mutex_lock(&lock[queueIndex]);
                    queues[queueIndex]->burst_length = queues[queueIndex]->burst_length - temp2->remaining_time;
                    temp2->remaining_time = 0;
                    gettimeofday(&tval_finish, NULL);
                    temp2->finish_time = time_diff(tval_start, tval_finish);
                    temp2->next = finishedBursts->next;
                    finishedBursts->next = temp2;
                    pthread_mutex_unlock(&lock[queueIndex]);
                }
            }
        }
    }
    pthread_exit(NULL);
}
void display_list(struct burst_node *head){
    if (head == NULL)
        printf("INVALID HEAD \n ");
    int a = 0;
    int count = 0;
    if (oFlag == 0){
        printf("pid\tcpu\tburstlen\tarv\tfinish\twaitingtime\tturnaround\n");
        while (head != NULL){
            printf("%d\t%d\t%d\t\t%d\t%d\t%d\t\t%d\n", head->process_id, head->processor_id, head->burst_length,
                   (head->arrival_time), (head->finish_time),
                   (head->finish_time - head->arrival_time) - head->burst_length, (head->finish_time - head->arrival_time));
            a += (head->finish_time - head->arrival_time);
            count++;
            head = head->next;
        }
        printf("Average turnaround time: %d ms", (int)(a / count));
    }
    if (oFlag == 1){ 
        pthread_mutex_lock(&writelock);
        // open the file for writing
         
        if (fp == NULL){
            fprintf(fp, "Error opening the file %s", outfile);
            exit(0);
        }
        // write to the text file
        fprintf(fp, "pid\tcpu\tburstlen\tarv\tfinish\twaitingtime\tturnaround\n");
        while (head != NULL){
            fprintf(fp, "%d\t%d\t%d\t\t\t%d\t%d\t\t%d\t\t\t%d\n", head->process_id, head->processor_id, head->burst_length,
                    (head->arrival_time), (head->finish_time),
                    (head->finish_time - head->arrival_time) - head->burst_length, (head->finish_time - head->arrival_time));
            a += (head->finish_time - head->arrival_time);
            count++;
            head = head->next;
        }
        fprintf(fp, "Average turnaround time: %d ms\n\n\n", (int)(a / count));
           pthread_mutex_unlock(&writelock);
    }
}
int main(int argc, char *argv[]){
    // argument parsing
    while ((opt = getopt(argc, argv, "n:a:s:i:o:r:m:")) != -1){
        switch (opt){
        case 'n':
            if (optarg[0] != '-'){
                n = atoi(optarg);
            }
            else{
                optind--;
            }
            break;
        case 'a':
            sap = optarg;
            qs = argv[optind++];
            break;
        case 's':
            alg = optarg;
            if (strcmp(alg, "RR") == 0){
                char *tempChar = argv[optind];
                if (tempChar[0] != '-'){
                    q = atoi(argv[optind]);
                }
            }
            if (strcmp(alg, "RR") != 0){
                q = 0;
            }
            break;
        case 'i':
            infile = optarg;
            iFlag = 1;
            break;
        case 'o':
            outfile = optarg;
            oFlag = 1;
            break;
        case 'r':
            t = atoi(optarg);
            t1 = atoi(argv[optind++]);
            t2 = atoi(argv[optind++]);
            l = atoi(argv[optind++]);
            l1 = atoi(argv[optind++]);
            l2 = atoi(argv[optind++]);
            PC = atoi(argv[optind++]);
            rFlag = 1;
            break;
        case 'm':
            mode = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-n N] [-a SAP QS] [-s ALG Q] [-i INFILE] [-o OUTFILE] [-r T T1 T2 L L1 L2] [-m OUTMODE]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    int numberOfQueues = 1;
    if (strcmp(sap, "M") == 0) // every processor has its own queue, else only one queue
        numberOfQueues = n;
    gettimeofday(&tval_start, NULL);
    pthread_attr_t attr;
    pthread_t *tid;
    struct t_param *param;
    tid = (pthread_t *)malloc(sizeof(pthread_t) * n);
    lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * numberOfQueues);
    queues = (struct burst_node **)malloc(sizeof(struct burst_node *) * numberOfQueues);
    finishedBursts = (struct burst_node *)malloc(sizeof(struct burst_node));
    for (int i = 0; i < numberOfQueues; i++){
        queues[i] = (struct burst_node *)malloc(sizeof(struct burst_node));
        queues[i]->burst_length = 0;
        queues[i]->dummy_flag = -1;
        queues[i]->next = NULL;
        pthread_mutex_init(&lock[i], NULL);
    }
    pthread_mutex_init(&writelock, NULL);
    for (int i = 0; i < n; i++){
        pthread_attr_init(&attr);
        param = malloc(sizeof(struct t_param));
        param->processor_id = i;
        param->numberOfQueues = numberOfQueues;
        int ret = pthread_create(&(tid[i]), &attr, process_simulator, (void *)param);
        if (ret != 0){
            printf("thread create failed \n");
            exit(1);
        }
    }
    if(oFlag == 1){
         fp = fopen(outfile, "a");
         fprintf(fp,"Simulation started with values:\nn = %d, SAP = %s, QS = %s, ALG = %s, Q = %d, infile = %s, outmode = %d, outfile = %s, T = %d, T1 = %d, T2 = %d, L = %d, L1 = %d, L2 = %d, PC = %d\n", n, sap, qs, alg, q, infile, mode, outfile, t, t1, t2, l, l1, l2,PC);
    }else{
        printf("Simulation started with values:\nn = %d, SAP = %s, QS = %s, ALG = %s, Q = %d, infile = %s, outmode = %d, outfile = %s, T = %d, T1 = %d, T2 = %d, L = %d, L1 = %d, L2 = %d, PC = %d\n", n, sap, qs, alg, q, infile, mode, outfile, t, t1, t2, l, l1, l2,PC);
    }
    // read file
    if (rFlag == 0 && iFlag == 1){
        FILE *ptr = fopen(infile, "r");
        if (ptr == NULL){
            printf("no such file.");
            return 0;
        }
        // reading file
        char buf[100];
        int counter = 1;
        int queue_select = 0;
        while (fscanf(ptr, "%*s %s", buf) == 1){
            // odd number counter means create burst.
            if (counter % 2 == 1){
                // queue adding method
                if (strcmp(sap, "S") == 0){
                    queue_select = 0;
                }
                else{
                    if (strcmp(qs, "RM") == 0){
                        queue_select = queue_select % numberOfQueues;
                    }
                    else{
                        queue_select = 0;
                        for (int i = 0; i < numberOfQueues - 1; i++){
                        if (queues[i + 1]->burst_length < queues[i]->burst_length)
                            queue_select = i + 1;
                        }
                    }
                }
                // pthread_mutex_lock(&lock[queue_select]);
                // struct burst_node* tempBurst = (struct burst_node*)malloc(sizeof(struct burst_node));
                //
                // if(queues[queue_select] != NULL){
                //     tempBurst = queues[queue_select]->next;
                //
                // }
                // pthread_mutex_unlock(&lock[queue_select]);
                // adding burst to the related queue
                struct burst_node *new = (struct burst_node *)malloc(sizeof(struct burst_node));
                gettimeofday(&(tval_finish), NULL);
                new->arrival_time = time_diff(tval_start, tval_finish);
                new->burst_length = atoi(buf);
                new->remaining_time = atoi(buf);
                new->process_id = counter - (counter / 2);
                new->processor_id = queue_select;
                pthread_mutex_lock(&lock[queue_select]);
                new->next = queues[queue_select]->next; // tempBurst;
                new->dummy_flag = 0;
                if (mode == 3){
                    if (oFlag == 0){
                        printf("Main thread added the burst that has the following information to the queue  %d:\n", queue_select + 1);
                        printf("time = %d, pid= %d, burstlen=%d\n\n",
                               new->arrival_time, new->process_id, new->burst_length);
                    }
                    else if (oFlag == 1){ 
                        pthread_mutex_lock(&writelock);
                        // open the file for writing
                         
                        if (fp == NULL){
                        printf("Error opening the file %s", outfile);
                        exit(0);
                        }
                        // write to the text file
                        fprintf(fp,"Main thread added the burst that has the following information to the queue  %d:\n", queue_select + 1);
                        fprintf(fp,"time = %d, pid= %d, burstlen=%d\n\n",
                                new->arrival_time, new->process_id, new->burst_length);
                        // close the file
                           pthread_mutex_unlock(&writelock);
                    }
                }
                queues[queue_select]->next = (new);
                queues[queue_select]->burst_length += atoi(buf); // first element in the linked burst items indicates the total burst length in the queue
                pthread_mutex_unlock(&lock[queue_select]);
                new = NULL;
                // tempBurst = NULL;
                free(new);
                // free(tempBurst);
                queue_select++;
            }
            else{
                usleep(atoi(buf) * USEC_VAL); // USEC_VAL usec = 1 millisecond
            }
            counter++;
        } // adding to the queues and sleeping for interarrival times done. need to add dummy item to queues.
        // adding dummy items
        fclose(ptr);
    }
    else{
        PC = (2 * PC) - 1;
        double mu;
        double x;
        int queue_select = 0;
        int counter = 1;
        while (PC > 0){
            double u = r2();
            if (PC % 2 == 1){
                mu = (double)(1 / (double)(l));
                x = (double)(((double)(-1) / (log(1 - u))) / mu);
                if (x <= l2 && x >= l1){
                    // queue adding method
                    if (strcmp(sap, "S") == 0){
                        queue_select = 0;
                    }
                    else{
                        if (strcmp(qs, "RM") == 0){
                        queue_select = queue_select % numberOfQueues;
                        }
                        else{
                        queue_select = 0;
                        for (int i = 0; i < numberOfQueues - 1; i++){
                            if (queues[i + 1]->burst_length < queues[i]->burst_length)
                                queue_select = i + 1;
                        }
                        }
                    }
                    struct burst_node *new = (struct burst_node *)malloc(sizeof(struct burst_node));
                    gettimeofday(&(tval_finish), NULL);
                    new->arrival_time = time_diff(tval_start, tval_finish);
                    new->burst_length = (int)x;
                    new->remaining_time = (int)x;
                    new->process_id = counter;
                    new->processor_id = queue_select;
                    pthread_mutex_lock(&lock[queue_select]);
                    new->next = queues[queue_select]->next; // tempBurst;
                    new->dummy_flag = 0;
                    if (mode == 3){
                        if (oFlag == 0){
                        printf("Main thread added the burst that has the following information to the queue of the CPU %d:\n", queue_select + 1);
                        printf("time = %d, pid= %d, burstlen=%d\n\n",
                               new->arrival_time, new->process_id, new->burst_length);
                        }
                        else if (oFlag == 1){ 
                            pthread_mutex_lock(&writelock);
                        
                        // write to the text file
                        fprintf(fp,"Main thread added the burst that has the following information to the queue of the CPU %d:\n", queue_select + 1);
                        fprintf(fp,"time = %d, pid= %d, burstlen=%d\n\n",
                                new->arrival_time, new->process_id, new->burst_length);
                        // close the file
                           pthread_mutex_unlock(&writelock);
                        }
                    }
                    queues[queue_select]->next = (new);
                    queues[queue_select]->burst_length += (int)x; // first element in the linked burst items indicates the total burst length in the queue
                    pthread_mutex_unlock(&lock[queue_select]);
                    new = NULL;
                    free(new);
                    counter++;
                    queue_select++;
                }
                else{
                    continue;
                }
                PC--;
            }
            else{
                mu = (double)(1 / (double)t);
                x = (double)(((-1) / (log(1 - u))) / mu);
                if (x <= t2 && x >= t1){
                    usleep(((int)x) * USEC_VAL); // USEC_VAL usec = 1 millisecond
                }
                else{
                    continue;
                }
                PC--;
            }
        }
    }
    for (int i = 0; i < numberOfQueues; i++){
        struct burst_node *new = (struct burst_node *)malloc(sizeof(struct burst_node));
        new->burst_length = -1;
        new->process_id = -1;
        new->processor_id = i;
        new->dummy_flag = 1;
        pthread_mutex_lock(&lock[i]);
        new->next = queues[i]->next;
        queues[i]->next = (new);
        pthread_mutex_unlock(&lock[i]);
        new = NULL;
        free(new);
    } // done with dummy items.
    // wait threads
    for (int i = 0; i < n; ++i){
        int ret = pthread_join(tid[i], NULL);
        if (ret != 0){
            printf("thread join failed \n");
            exit(1);
        }
    }
    for (int i = 0; i < numberOfQueues; i++){
        free(queues[i]);
    }
    free(lock);
    free(queues);
    finishedBursts = sort_list(finishedBursts);
    printf("\n\n");
    display_list(finishedBursts->next);
    if(oFlag == 1){
        fclose(fp);
    }
    return 0;
}