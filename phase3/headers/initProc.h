#ifndef INIT_PROC_H
#define INIT_PROC_H

#include "../../phase2/headers/dipendenze.h"


#define STACKVPN (ENTRYHI_VPN_MASK >> VPNSHIFT)
#define DBit 2 // Dirty bit
#define VBit 1 // Valid bit
#define GBit 0 // Global bit

//Aggiunto 
extern void uTLB_ExceptionHandler(); 
extern void generalExceptionHandler();
unsigned int getPageIndex(unsigned int entry_hi);
void initPageTable(support_t *sup, int asid);
extern void print(char *msg);
#endif