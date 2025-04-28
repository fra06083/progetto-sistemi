#ifndef INITIAL_H
#define INITIAL_H

#include "../../phase1/headers/pcb.h"
#include "../../phase1/headers/asl.h"
#include "../../headers/const.h"
#include "../../klog.c"

#include "dipendenze.h"

#include "../functions.c"
#include "../exceptions.c"
#include "../interrupts.c"
#include "../scheduler.c"

//INIZIALIZZAZIONE DEL NUCLEO
/*
Declare the Level 3 global variables. This should include:
    • Process Count: integer indicating the number of started, but not yet terminated processes.
    • Ready Queue: A queue of PCBs that are in the “ready” state.
    • Current Process: Vector of pointers to the PCB that is in the “running” state on each CPU,
    i.e. the i-th element of Current Process is the executing process on i-th CPU.
    • Device Semaphores: The Nucleus maintains one integer semaphore for each external (sub)device,
    plus one additional semaphore to support the Pseudo-clock. Since terminal devices are actually two independent sub-devices, the Nucleus maintains two semaphores for each terminal
    device.
    • Global Lock: A integer that can have only two possible value 0 and 1, used used for
    synchronization between different instances of the nucleus running on different CPUs.
*/
#define BASE_STACK0 (memaddr) 0x2000200           // Inizio dello stack
int process_count = 0;                  // Contatore dei processi
struct pcb_t *current_process[NCPU];    // Vettore di puntatori, 8 processi che vanno nelle varie CPU
struct semd_t sem[NRSEMAPHORES];
volatile unsigned int global_lock = 1;      // Lock globale
struct list_head pcbReady;              // Lista dei processi pronti
cpu_t start_time[NCPU]; // tempo di inizio di ogni processo
// extern perché sennò darebbe errore il compilatore
// extern fa capire solamente che la funzione è definita in un altro file
extern void test();
extern void uTLB_RefillHandler();
// NOSTRE FUNZIONI
void initializeSystem();
void configurePassupVector();
void configureIRT();
void createFirstProcess();
void configureCPUs();

#endif
