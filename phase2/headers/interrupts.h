#ifndef INT_H_INCLUDED
#define INT_H_INCLUDED
#include "dipendenze.h"
#include "../../headers/types.h"

// NOSTRE DUE define, il primo ci dà il bit map del dispositivo, il secondo ci dà il numero 48 che è il sem dello pseudo clock
#define BITMAP(IntlineNo) (*(memaddr *)(CDEV_BITMAP_BASE + ((IntlineNo) - 3) * WS))
#define PSEUDOSEM (NRSEMAPHORES - 1)
// Dichiarazioni extern per variabili definite in initial.h
extern pcb_t *current_process[NCPU]; 
extern struct list_head pcbReady;
extern volatile unsigned int global_lock;
extern int sem[SEMDEVLEN];
// Prototipi funzioni
int getDevNo(int IntlineNo);
int getIntLineNo(int intCode);
void interruptHandler(int intCode, state_t *stato);
void handleDeviceInterrupt(int intLineNo, int devNo);
void handlePLTInterrupt();
void handleIntervalTimerInterrupt();

#endif