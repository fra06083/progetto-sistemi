#ifndef EXC_VM_INCLUDED
#define EXC_VM_INCLUDED
#include "sysSupport.h"
#include "initProc.h"

extern int asidAcquired;     
extern int swap_mutex;             
extern swap_t swap_pool[POOLSIZE]; // Swap Pool table

#define VBit 1 // Valid bit
#define MAXBLOCK 24  //SERVE???? Flash Device Block Numbers [0 ... MAXBLOCK - 1]
void updateTLB(pteEntry_t *pte); // Function to update the TLB with a new PTE entry
extern void supportTrapHandler(int asid);
extern void acquire_mutexTable(int asid);
extern void release_mutexTable();
void uTLB_ExceptionHandler();
int selectSwapFrame();
#endif
