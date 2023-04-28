#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>          
#include <stdio.h> 
#include <stdlib.h> 
#include <pthread.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#define MEM_NAME "/sharedmem"

//GLB VARS


int fileNumber;
char** fileNames;
int topK;
int N;
struct Node { 
    char * word; 
    struct Node* next; 
    int count;
};
struct Node * head1 = NULL; 
struct Node * head2 = NULL; 
struct Node * head3 = NULL; 
struct Node * head4 = NULL; 
struct Node * head5 = NULL; 
struct Node * head6 = NULL; 
struct Node * head7 = NULL; 
struct Node * head8 = NULL; 
struct Node * head9 = NULL; 
struct Node * head10 = NULL; 
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
void *runner(void* param){
	FILE *fp;
	char line[128];
	struct Node *headthread = NULL;
    fp = fopen(param, "r");
    char* word;
    while (fscanf(fp, " %1023s", line) == 1){
        word = strdup(line);
        toUpperCase(&word);
        push(&headthread,word,1);
    }
    fclose(fp);
    sortNodes(&headthread);
    struct Node *headthreadtops = NULL;
    for(int i = 0; i < topK; i++){
        push(&headthreadtops,headthread->word,headthread->count);
        headthread = headthread->next;
    }
    if(fileNumber == 1){
	 head1 = headthreadtops;
	}else if(fileNumber == 2){
	 head2 = headthreadtops;
	}else if(fileNumber == 3){
	 head3 = headthreadtops;
	}else if(fileNumber == 4){
	 head4 = headthreadtops;
	}else if(fileNumber == 5){
	 head5 = headthreadtops;
	}else if(fileNumber == 6){
	 head6 = headthreadtops;
	}else if(fileNumber == 7){
	 head7 = headthreadtops;
	}else if(fileNumber == 8){
	 head8 = headthreadtops;
	}else if(fileNumber == 9){
	 head9 = headthreadtops;
	}else if(fileNumber == 10){
	 head10 = headthreadtops;
	}
	pthread_exit(0); 
   
}
void writer(char * fileName,char * word,int count){

   FILE *fp;
   fp = fopen(fileName, "a+");
   fprintf(fp,"%s %d \n",word,count);
   fclose(fp);

}

int main(int argc, char* argv[]){
    clock_t start_time = clock();
    topK = atoi(argv[1]);
    N = atoi(argv[3]);
    if(argc < 5){
        printf("argument count can not be < 5");
        return 1;
    }else if (argc > 14){
        printf("argument count can not be > 14");
        return 1;
    }
    struct Node* mainhead = NULL;
    pthread_t tid; /* id of the created thread */
	pthread_attr_t attr;  /* set of thread attributes */
    fileNumber = (int)(atoi(argv[3]));

	if(argc >=5 ){
        fileNumber = 1;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[4]); 
        pthread_join (tid, NULL);
	}
	if(argc >=6 ){
        fileNumber = 2;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[5]); 
        pthread_join (tid, NULL);
	}
	if(argc >=7 ){
        fileNumber = 3;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[6]); 
        pthread_join (tid, NULL);
	}
	if(argc >=8 ){
        fileNumber = 4;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[7]); 
        pthread_join (tid, NULL);
	}
	if(argc >=9 ){
        fileNumber = 5;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[8]); 
        pthread_join (tid, NULL);
	}
    if(argc >=10 ){
        fileNumber = 6;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[9]); 
        pthread_join (tid, NULL);
	}
    if(argc >=11 ){
        fileNumber = 7;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[10]); 
        pthread_join (tid, NULL);
	}
    if(argc >=12 ){
        fileNumber = 8;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[11]); 
        pthread_join (tid, NULL);
	}
    if(argc >=13 ){
        fileNumber = 9;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[12]); 
        pthread_join (tid, NULL);
	}
    if(argc >=14 ){
        fileNumber = 10;
        pthread_attr_init (&attr); 	
        pthread_create (&tid, &attr, runner, argv[13]); 
        pthread_join (tid, NULL);
	}
    
    struct Node* temp = NULL;
    temp = head1;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    temp = head2;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    temp = head3;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    temp = head4;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    temp = head5;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    temp = head6;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    temp = head7;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    temp = head8;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    temp = head9;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    temp = head10;
    while(temp != NULL){
        push(&mainhead,temp->word,temp->count);
        temp = temp->next;
    }
    sortNodes(&mainhead);

    struct Node *temp2 = mainhead;
    int topKCounter = 0; 
    while(temp2 != NULL &&topKCounter < atoi(argv[1])){ 
	    writer(argv[2],temp2->word,temp2->count);	
	    temp2 = temp2->next; 
        topKCounter++;
	}  

    clock_t end_time = clock();

    printf("The program took: %zd milliseconds.\n", end_time - start_time);
        
    return 0;
}



