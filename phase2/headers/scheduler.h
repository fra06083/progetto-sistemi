#ifndef SCH_H_INCLUDED
#define SCH_H_INCLUDED
#include "dipendenze.h"
extern int process_count;
extern struct pcb_t *current_process[NCPU];
extern struct list_head pcbReady;
extern int sem[SEMDEVLEN];
extern volatile unsigned int global_lock;
extern cpu_t start_time[NCPU];
// METODI
void scheduler();
#endif