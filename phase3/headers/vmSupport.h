#ifndef EXC_VM_INCLUDED
#define EXC_VM_INCLUDED
#include "sysSupport.h"
#include "initProc.h"

extern int asidAcquired;     
extern int swap_mutex;             

static int next_frame_index = 0; // Static x FIFO round-robin

extern swap_t swap_pool[POOLSIZE]; // Swap Pool table

#define VBit 1 // Valid bit
#define MAXBLOCK 24  //SERVE???? Flash Device Block Numbers [0 ... MAXBLOCK - 1]
void updateTLB(pteEntry_t *pte); // Function to update the TLB with a new PTE entry
extern void supportTrapHandler(support_t *sup_ptr);
extern void acquire_mutexTable(int asid);
extern void release_mutexTable();
void uTLB_ExceptionHandler();
int selectSwapFrame();
void acquire_mutexTable(int asid);
void release_mutexTable();
#endif
