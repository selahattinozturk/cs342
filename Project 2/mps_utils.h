#include <stdlib.h>
#include <mqueue.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <mqueue.h>
#include <sys/time.h>


struct burst_node{
    struct burst_node* next;
    int process_id;
    int burst_length; 
    int arrival_time;
    int remaining_time;
    int finish_time;
    int turnaround_time;
    int processor_id;
    int dummy_flag;
};

struct t_param{
    int processor_id;
    int numberOfQueues;
};