#ifndef SCH_H_INCLUDED
#define SCH_H_INCLUDED

extern int process_count;
extern struct pcb_t *current_process[NCPU];
extern struct list_head pcbReady;
extern unsigned int getPRID();
extern volatile unsigned int global_lock;
extern int lock_0;
// METODI
void scheduler();
#endif