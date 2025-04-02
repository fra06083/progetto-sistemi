#ifndef EXC_H_INCLUDED
#define EXC_H_INCLUDED

extern void scheduler();
extern volatile unsigned int global_lock;
extern int process_count;
extern struct list_head ready_queue;
extern struct semd_t sem[NRSEMAPHORES];
extern struct pcb_t *current_process[NCPU];

// Funzioni dichiarate
void exceptionHandler(); 
void syscallHandler();
void uTLB_ExceptionHandler();
#endif
