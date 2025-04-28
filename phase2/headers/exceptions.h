#ifndef EXC_H_INCLUDED
#define EXC_H_INCLUDED
#include "dipendenze.h"
#include "interrupts.h"
#include "functions.h"
#include "../../phase1/headers/asl.h"
#include "../../phase1/headers/pcb.h"
extern void scheduler();
extern volatile unsigned int global_lock;
extern int process_count;
extern struct list_head ready_queue;
extern semd_t sem[NRSEMAPHORES];
extern struct pcb_t *current_process[NCPU];
extern struct list_head pcbReady;
extern cpu_t start_time[NCPU];

// Funzioni dichiarate
void exceptionHandler(); 
void syscallHandler(state_t *stato);
void uTLB_ExceptionHandler();
void terminateProcess(state_t *c_state, unsigned int p_id);
void createProcess(state_t *c_state);
#endif
