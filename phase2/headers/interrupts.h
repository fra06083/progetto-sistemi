#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "../../headers/const.h"
#include "../../headers/listx.h"
#include "../../phase1/headers/pcb.h"
#include "initial.h"

// Dichiarazioni extern per variabili definite in initial.h
extern pcb_t *currentProcess[NCPU]; 
extern struct list_head readyQueue;
extern volatile unsigned int globalLock;

// Prototipi funzioni
void interruptHandler();
void handleDeviceInterrupt(int intLineNo, int devNo);
void handlePLTInterrupt();
void handleIntervalTimerInterrupt();

#endif