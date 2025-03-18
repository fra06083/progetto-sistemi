#ifndef EXC_H_INCLUDED
#define EXC_H_INCLUDED

#include "../../headers/listx.h"
#include "../../headers/types.h"
#include <uriscv/cpu.h>
#include <uriscv/arch.h>
#include <uriscv/types.h>
#include <uriscv/liburiscv.h> // libreria di uriscv, viene richiesta, sennò non abbiamo le funzioni che ci servono LDST
#include "../../phase1/headers/pcb.h"
// Funzioni esterne
extern unsigned int getPRID(void);
extern unsigned int setENTRYHI(unsigned int entry);
extern unsigned int setENTRYLO(unsigned int entry);
extern unsigned int LDST(void* state);
extern void uTLB_RefillHandler();
extern int getCause();
extern volatile unsigned int global_lock;
extern int process_count;
extern struct list_head ready_queue;
extern struct pcb_t *current_process[NCPU];  // Vettore di puntatori, 8 processi che vanno nelle varie CPU
extern struct semd_t sem[NRSEMAPHORES];
extern volatile unsigned int global_lock; // Lock globale
extern struct list_head pcbReady;         // Lista dei processi pronti
extern void uTLB_RefillHandler();

// Funzioni dichiarate
void exceptionHandler(); 
void syscallHandler();
void uTLB_ExceptionHandler();
#endif
