#ifndef FUNCS_H_INCLUDED
#define FUNCS_H_INCLUDED


extern pcb_t *current_process[NCPU]; 
extern struct list_head pcbReady;
extern volatile unsigned int global_Lock;
extern int process_count;
extern struct semd_t semd_h;

pcb_t *findProcess(int pid, int remove);
void killProcess(pcb_t *process);
#endif