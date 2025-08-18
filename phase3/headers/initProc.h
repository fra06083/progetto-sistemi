#ifndef INIT_PROC_H
#define INIT_PROC_H

#include "../../headers/const.h"
#include "../../headers/types.h"
#include "./uriscv/cpu.h"


#define STACKVPN (ENTRYHI_VPN_MASK >> VPNSHIFT)
#define DBit 2 // Dirty bit
#define VBit 1 // Valid bit
#define GBit 0 // Global bit

//Dichiarazioni (globali) delle variabili di fase 3 (qua)
//N.B. Nella documentazione in alcuni casi possiamo scegliere di dichiararle localmente nei file! 
int masterSem = 0; // alla fine dice di gestirlo così
state_t procStates[UPROCMAX];
support_t procSupport[UPROCMAX];

//Aggiunto 
extern void uTLB_ExceptionHandler(); 

extern void generalExceptionHandler();

support_t sup_struct[8]; //8 U-proc, per ogni  struttura di supporto 
swap_t swap_pool[POOLSIZE]; // nel documento dice uprocmax * 2
int swap_mutex = 1; // semaforo mutua esclusione pool
int asidAcquired = -1; // asid che prende la mutua esclusione
int supportSem[NSUPPSEM]; // Dal punto 9 ci servono dei semafori supporto dei device
int supportSemAsid[UPROCMAX];


unsigned int getPageIndex(unsigned int entry_hi);
void initPageTable(support_t *sup, int asid);
void p3test();
#endif