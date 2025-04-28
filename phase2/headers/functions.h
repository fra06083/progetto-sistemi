#ifndef FUNCS_H_INCLUDED
#define FUNCS_H_INCLUDED
#include "dipendenze.h"

extern pcb_t *current_process[NCPU]; 
extern struct list_head pcbReady;
extern volatile unsigned int global_lock;
extern int process_count;
extern struct semd_t semd_h;
extern semd_t sem[NRSEMAPHORES];

pcb_t *findProcess(int pid, int remove);
void killProcess(pcb_t *process);
void* memcpy(void* dest, const void* src, unsigned int len);
int findDevice(memaddr* indirizzo_comando);
#endif