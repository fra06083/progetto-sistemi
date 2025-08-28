#ifndef SYS_SUPPORT_H
#define SYS_SUPPORT_H
#include "initProc.h"
#include "vmSupport.h"
#include "../../phase2/headers/functions.h"
extern int masterSem; // alla fine dice di gestirlo così
extern state_t procStates[UPROCMAX];
extern support_t procSupport[UPROCMAX];
extern support_t sup_struct[8]; //8 U-proc, per ogni  struttura di supporto 
extern swap_t swap_pool[POOLSIZE]; // nel documento dice uprocmax * 2
extern int swap_mutex; // semaforo mutua esclusione pool
extern int asidAcquired; // asid che prende la mutua esclusione
extern int supportSem[NSUPPSEM]; // Dal punto 9 ci servono dei semafori supporto dei device
extern int supportSemAsid[UPROCMAX];
int printTerminal(char* msg, int lenMsg, int term);
int printPrinter(char* msg, int length, int devNo);
int inputFromTerminal(char* addr, int term);

// Funzioni di gestione dispositivi
void writeDevice(state_t *stato, int asid, int type);
void readTerminal(state_t* stato, int asid);

// Funzioni di gestione processi e trap
void TerminateSYS(int asidTerminate);
void generalExceptionHandler();
void supportTrapHandler(int asid);
#endif