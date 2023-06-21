#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include "rm.h"

#define NUMR 5        // number of resource types
#define NUMP 3        // number of threads

int AVOID = 1;
int exist[5] =  {2,2,2,2,2};  // resources existing in the system
int countThread1;
int countThread2;
int countThread3;
void pr (int tid, char astr[], int m, int r[])
{
    int i;
    printf ("thread %d, %s, [", tid, astr);
    for (i=0; i<m; ++i) {
        if (i==(m-1))
            printf ("%d", r[i]);
        else
            printf ("%d,", r[i]);
    }
    printf ("]\n");
}


void setarray (int r[MAXR], int m, ...)
{
    va_list valist;
    int i;
    
    va_start (valist, m);
    for (i = 0; i < m; i++) {
        r[i] = va_arg(valist, int);
    }
    va_end(valist);
    return;
}


void *threadfunc1 (void *a)
{
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int claim[MAXR];
    countThread1 = 0;
    
    tid = *((int*)a);
    rm_thread_started (tid);
    countThread1 = 1;
    setarray(claim, NUMR, 1,2,2,1,1);
    rm_claim (claim);
    
    setarray(request1, NUMR, 0,1,0,1,1);
    rm_request (request1);

    sleep(3);

    setarray(request2, NUMR, 1,1,2,0,0);
    rm_request (request2);

    rm_release (request1);
    rm_release (request2);

    rm_thread_ended();
    countThread1 =0;
    pthread_exit(NULL);
}


void *threadfunc2 (void *a)
{
    countThread2 =0;
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int request3[MAXR];
    int claim[MAXR];

    tid = *((int*)a);
    rm_thread_started (tid);
    countThread2 =1;
    setarray(claim, NUMR, 2,1,2,1,2);
    rm_claim (claim);
    
    setarray(request1, NUMR, 2,1,0,0,1);

    rm_request (request1);


    sleep(3);
    
    setarray(request2, NUMR, 0,0,1,1,1);

    rm_request (request2);

    sleep(3);
    
    setarray(request3, NUMR, 0,0,1,0,0);

    rm_request (request3);

    rm_release (request1);
    rm_release (request2);
    rm_release (request3);
    rm_thread_ended ();
    countThread2 =0;
    pthread_exit(NULL);
}



void *threadfunc3 (void *a)
{
    countThread3 =0;
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int claim[MAXR];
    tid = *((int*)a);
    rm_thread_started (tid);
    countThread2 =1;
    setarray(claim, NUMR, 1,2,1,1,2);
    rm_claim (claim);
    
    setarray(request1, NUMR, 0,2,0,1,1);

    rm_request (request1);
    sleep(3);

    setarray(request2, NUMR, 1,0,1,0,1);

    rm_request (request2);

    rm_release (request1);
    rm_release (request2);

    rm_thread_ended();
    countThread3 =0;
    pthread_exit(NULL);
}




int main(int argc, char **argv)
{
    int i;
    int tids[NUMP];
    pthread_t threadArray[NUMP];
    int count;
    int ret;
    if (argc != 2) {
        printf ("usage: ./app avoidflag\n");
        exit (1);
    }

    AVOID = atoi (argv[1]);
    
    if (AVOID == 1)
        rm_init (NUMP, NUMR, exist, 1);
    else
        rm_init (NUMP, NUMR, exist, 0);

    i = 0;  // we select a tid for the thread
    tids[i] = i;
    pthread_create (&(threadArray[i]), NULL,
                    (void *) threadfunc1, (void *)
                    (void*)&tids[i]);
    
    i = 1;  // we select a tid for the thread
    tids[i] = i;
    pthread_create (&(threadArray[i]), NULL,
                    (void *) threadfunc2, (void *)
                    (void*)&tids[i]);

    i = 2;  // we select a tid for the thread
    tids[i] = i;
    pthread_create (&(threadArray[i]), NULL,
                    (void *) threadfunc3, (void *)
                    (void*)&tids[i]);

    count = 0;
    if(atoi(argv[1]) == 0){//Deadlock

        while ( (countThread2 != 0 || countThread1 != 0) || countThread3 != 0) {
            sleep(1);
            rm_print_state("The current state");
            ret = rm_detection();
            if (ret > 0) {
                printf ("deadlock detected, count=%d\n", ret);
                rm_print_state("state after deadlock");
                printf("\nDue to deadlock, processes can not continue... Exiting\n");
                return 0;
            }
            count++;
        }
        
        if (ret == 0) {
            for (i = 0; i < NUMP; ++i) {
                pthread_join (threadArray[i], NULL);
                printf ("joined\n");
            }
        }
    }else if (atoi(argv[1]) == 1){//Avoid
        while ( (countThread2 != 0 || countThread1 != 0) || countThread3 != 0) {
            sleep(1);
            rm_print_state("The current state");
            ret = rm_detection();
            if (ret > 0) {
                printf ("deadlock detected, count=%d\n", ret);
                rm_print_state("state after deadlock");
                printf("\nDue to deadlock, processes can not continue... Exiting\n");
                return 0;
            }
            count++;
        }
        
        if (ret == 0) {
            for (i = 0; i < NUMP; ++i) {
                pthread_join (threadArray[i], NULL);
                printf ("joined\n");
            }
        }
        rm_print_state("After all the joins");
    }
    return 0;
}


