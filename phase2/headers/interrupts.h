#ifndef INT_H_INCLUDED
#define INT_H_INCLUDED
#include "dipendenze.h"
#include "../../headers/types.h"

// Dichiarazioni extern per variabili definite in initial.h
extern pcb_t *current_process[NCPU]; 
extern struct list_head pcbReady;
extern volatile unsigned int global_Lock;
extern semd_t sem[];
// Prototipi funzioni
void interruptHandler();
void handleDeviceInterrupt(int intLineNo, int devNo);
void handlePLTInterrupt();
void handleIntervalTimerInterrupt();

#endif