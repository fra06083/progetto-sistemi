#ifndef SCH_H_INCLUDED
#define SCH_H_INCLUDED

extern int process_count;
extern struct pcb_t *current_process[NCPU];
extern struct list_head pcbReady;
extern unsigned int getPRID();
extern volatile unsigned int global_lock;
extern int lock_cpu0;
// METODI
void scheduler();
void terminateProcess(state_t *c_state, unsigned int p_id);
#endif