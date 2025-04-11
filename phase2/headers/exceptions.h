#ifndef EXC_H_INCLUDED
#define EXC_H_INCLUDED
#include "interrupts.h"
extern void scheduler();
extern volatile unsigned int global_lock;
extern int process_count;
extern struct list_head ready_queue;
extern struct semd_t sem[NRSEMAPHORES];
extern struct pcb_t *current_process[NCPU];
extern int lock_cpu0;
// Funzioni dichiarate
void exceptionHandler(); 
void syscallHandler();
void uTLB_ExceptionHandler();
void terminateProcess(state_t *c_state, unsigned int p_id);
void createProcess(state_t *c_state);
extern void uTLB_RefillHandler();
#endif
