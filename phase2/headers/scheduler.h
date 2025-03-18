#ifndef SCH_H_INCLUDED
#define SCH_H_INCLUDED

#include "../../headers/listx.h"
#include "../../headers/types.h"
extern int process_count;
extern struct pcb_t *current_process[NCPU];
extern pcb_t* removeProcQ(struct list_head* head);
extern struct list_head pcbReady;
extern unsigned int getPRID();
extern volatile unsigned int global_lock;
extern 
// METODI
void scheduler();
#endif