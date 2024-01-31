#include "pcb.h"
#include <stdlib.h>
#include <stdio.h>
#define MAX_OPEN_FILES 128

PCB* init_pcb(pid_t pid, pid_t parent, int src_open_fds[MAX_OPEN_FILES], int num_open_fds, int priority) {
    PCB* pcb = malloc(sizeof(PCB));
    pcb -> uc = malloc(sizeof(ucontext_t));
    
    pcb -> pid = pid;
    pcb -> parent_pid = parent;
    // pcb -> children_pids = NULL;
    pcb->children_pids = malloc(sizeof(pid_t) * 100);

    pcb -> num_children = 0;
    memcpy(pcb->open_fds, src_open_fds, sizeof(src_open_fds[0]) * MAX_OPEN_FILES);
    pcb -> num_open_fds = num_open_fds;
    pcb -> priority = priority;
    pcb -> status = READY;
    pcb ->is_background = false;

    pcb->process_name = malloc(sizeof(char) * 20);

    pcb -> waitpid_pid = 0;
    pcb->sleep_counter = -1;
    // pcb -> sleep_fin_time = 0;
    pcb->e_status = NOT_EXITED;
    return pcb;
}

void free_pcb(PCB* pcb) {
    free(pcb->children_pids);
    // need to free ucontext?
}