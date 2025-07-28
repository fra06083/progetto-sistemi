/*
- TLB exception handler (The Pager) [Section 4].
- function(s) for reading and writing flash devices
- the Swap Pool table is local to this module
*/
#include "../headers/const.h"
#include "../headers/types.h"
#include "uriscv/arch.h"
#include "uriscv/cpu.h"
extern int asidAcquired;
extern int swap_mutex;
void acquire_mutexTable(int asid){
    SYSCALL(PASSEREN, (int)&swap_mutex, 0, 0);
    asidAcquired = asid;
}
void release_mutexTable(){
    SYSCALL(VERHOGEN, (int)&swap_mutex, 0, 0);
    asidAcquired = -1;
}
void uTLB_ExceptionHandler() {
    support_t *sup_ptr = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0); 
    state_t *state = &(sup_ptr->sup_exceptState[PGFAULTEXCEPT]);

    if (state->cause == EXC_MOD) { // punto 4.2
      //  supportTrapHandler(supp->sup_asid); da creare
    }
    unsigned int p = getPageIndex(state->entry_hi);
    unsigned int ASID = sup_ptr->sup_asid;
    acquire_mutexTable(ASID);
    int trovato = FALSE;
    for (int i = 0; i < POOLSIZE && !trovato; i++){
        unsigned int sw_asid = swap_pool[i].sw_asid;
        if (sw_asid == ASID && swap_pool[i].sw_pageNo == p)
            trovato = TRUE;
    } 
    if (trovato) {
        updateTLB(swap_pool[i].sw_pte);
        if (sup_ptr->sup_privatePgTbl[p].pte_entryLO & ENTRYLO_VALID) { // controlliamo se è valido, rilasciamo e ricarichiamo
            release_mutexTable();
            LDST(state);
        } 
    }
    // ... continua punto 7
}