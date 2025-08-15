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


#define VBit 1 // Valid bit


static int next_frame_index = 0; // Static x FIFO round-robin

int selectSwapFrame(){                                    // Page replacement FIFO (5.4)
    int selected_frame = next_frame_index;
    next_frame_index = (next_frame_index + 1) % POOLSIZE; // "increment this variable mod the size of the Swap Pool"
    return selected_frame;
}

void acquire_mutexTable(int asid){
    SYSCALL(PASSEREN, (int)&swap_mutex, 0, 0);
    asidAcquired = asid;
}
void release_mutexTable(){
    SYSCALL(VERHOGEN, (int)&swap_mutex, 0, 0);
    asidAcquired = -1;
}

void supportTrapHandler(support_t sup_ptr){         
   //Section 8 ... 


}


void uTLB_ExceptionHandler() {
    support_t *sup_ptr = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);       //NSYS8 (pulire commenti, uso come placeholder)
    state_t *state = &(sup_ptr->sup_exceptState[PGFAULTEXCEPT]);             //Cause of the TLB Exception (4.2)

    if (state->cause == EXC_MOD) { // punto 4.2
      //  supportTrapHandler(supp->sup_asid); da creare
    }
    unsigned int p = getPageIndex(state->entry_hi);                          // punto 5 (4.2 Pager)
    unsigned int ASID = sup_ptr->sup_asid;                                   // ASID: asid current process
    acquire_mutexTable(ASID);                                                // punto 4 (4.2 Pager)
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
        // Seleziona un frame dalla Swap Pool usando l'algoritmo di page replacement
        unsigned int fr_index = selectSwapFrame();     //FCFS() recommended/RR();
        acquire_mutexTable(ASID); 
        if(swap_pool[fr_index].sw_asid != -1 ){                         //asid -1 -> frame libero 
            //frame occupato, lo libero (punto 9)
            int k = swap_pool[fr_index].sw_pageNo;                     //Virtual Page Number k del frame occupato 
            int asid_proc_x = swap_pool[fr_index].sw_asid;             //k e processo x (asid) ~ stessi nomi documentazione 
            pteEntry_t *proc_x_privatePgTbl = swap_pool[fr_index].sw_pte;   
            proc_x_privatePgTbl[k].pte_entryHI = (0 << VBit);           //Shift sx, spengo il Vbit per rendere invalid  
            //punto 9 (b): Update TLB if needed 
            int trovato = FALSE;             
            for (int i = 0; i < POOLSIZE && !trovato; i++){             //Da telegram: swap_mutex così l'esecuzione è atomica 9 (a,b), manca disabilitazione interrupt in questa fase 
                unsigned int sw_asid = swap_pool[i].sw_asid;
                if (sw_asid == proc_x_privatePgTbl && swap_pool[i].sw_pageNo == k)
                    trovato = TRUE;
            } 
            if(trovato) updateTLB(swap_pool[fr_index].sw_pte);
            //Punto 9 (c)
            

        }
}
