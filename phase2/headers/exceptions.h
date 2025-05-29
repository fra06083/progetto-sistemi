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
extern int sem[SEMDEVLEN];
extern struct pcb_t *current_process[NCPU];
extern struct list_head pcbReady;
extern cpu_t start_time[NCPU];

// Funzioni dichiarate
void exceptionHandler();
void passupordie(int cause, state_t *stato);
void syscallHandler(state_t *stato);
void terminateProcess(state_t *c_state, unsigned int p_id);
void createProcess(state_t *c_state);
void P(state_t* stato, unsigned int p_id);
void V(state_t* stato, unsigned int p_id);
void DoIO(state_t *stato, unsigned int p_id);
#endif
