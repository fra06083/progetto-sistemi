#include "./headers/vmSupport.h"

#define getFlashAddr(asid) (DEV_REG_ADDR(IL_FLASH, asid-1))
#define SWAP_POOL_START (RAMSTART + (64 * PAGESIZE) + (NCPU * PAGESIZE))

/*
- TLB exception handler (The Pager) [Section 4].
- function(s) for reading and writing flash devices
- the Swap Pool table is local to this module
*/

// POLITICA PER RIMPIAZZAMENTO FRAME SWAP POOL
int selectSwapFrame(){     
    static int next_frame_index = 0; // Page replacement FIFO (5.4) 
    for (int i = 0; i < POOLSIZE; i++) {
        if (swap_pool[i].sw_asid == -1) {
            return i; // frame libero
        }
    }
    return ((next_frame_index++) % POOLSIZE); // round-robin se nessun frame libero
}

// Aggiorna la TLB con mutua esclusione implicita
void check_updateTLB(pteEntry_t *pte){
    setENTRYHI(pte->pte_entryHI);
    TLBP();
    unsigned int index = getINDEX();
    if(!(index & PRESENTFLAG)){ 
        setENTRYLO(pte->pte_entryLO);
        TLBWI();
    }
}

// Dispatch I/O su flash device con protezione dei semafori
void FlashRW(int asid, memaddr frameAddr, int block, int read){
    int semIndex = findDevice((memaddr*) getFlashAddr(asid));
    acquireDevice(asid, semIndex);
    dtpreg_t *devreg = (dtpreg_t *) getFlashAddr(asid);
    int commandAddr = (int)&devreg->command;
    int commandValue = (read ? FLASHREAD : FLASHWRITE) | (block << 8);
    devreg->data0 = frameAddr;
    int status = SYSCALL(DOIO, commandAddr, commandValue, 0);
    releaseDevice(asid, semIndex);

    int error = read ? 4 : 5;
    if ((status & 0xFF) == error) { 
        release_mutexTable(); // assicura rilascio swap pool
        supportTrapHandler(asid);
    }
}

void writeFlash(int asid, memaddr frameAddr, int block){
    FlashRW(asid, frameAddr, block, 0);
}

void readFlash(int asid, memaddr frameAddr, int block){
    FlashRW(asid, frameAddr, block, 1);
}

// TLB Exception Handler (The Pager)
void uTLB_ExceptionHandler() {  
    support_t *sup_ptr = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);       
    state_t *state = &(sup_ptr->sup_exceptState[PGFAULTEXCEPT]); // Cause of TLB Exception

    if (state->cause == EXC_MOD) { // punto 4.2
        supportTrapHandler(sup_ptr->sup_asid);
        return;
    }

    unsigned int p = getPageIndex(state->entry_hi); // punto 5
    unsigned int ASID = sup_ptr->sup_asid;
    acquire_mutexTable(ASID); // punto 4 (Pager)

    int found = FALSE;
    int i;    
    for (i = 0; i < POOLSIZE && !found; i++){
        if (swap_pool[i].sw_asid == ASID && swap_pool[i].sw_pageNo == p){
            found = TRUE;
            break;
        }   
    }

    if (found) {  
        check_updateTLB(swap_pool[i].sw_pte);                           
        if (sup_ptr->sup_privatePgTbl[p].pte_entryLO & ENTRYLO_VALID) { 
            release_mutexTable();
            LDST(state);
        }
    } 

    // Seleziona un frame dalla Swap Pool
    int fr_index = selectSwapFrame();    

    if(swap_pool[fr_index].sw_asid != -1){ // frame occupato
        int k = swap_pool[fr_index].sw_pageNo;
        int asid_proc_x = swap_pool[fr_index].sw_asid;
        pteEntry_t *entry = swap_pool[fr_index].sw_pte;
        entry->pte_entryLO &= ~VALIDON; // invalida la pagina
        check_updateTLB(entry); // aggiorna TLB se necessario
        writeFlash(asid_proc_x, SWAP_POOL_START + (fr_index * PAGESIZE), k);
    } 

    readFlash(ASID, SWAP_POOL_START + (fr_index * PAGESIZE), p);
    
    // Aggiorna swap pool table
    swap_pool[fr_index].sw_asid = ASID;
    swap_pool[fr_index].sw_pageNo = p;
    swap_pool[fr_index].sw_pte = &(sup_ptr->sup_privatePgTbl[p]);
    swap_pool[fr_index].sw_pte->pte_entryLO |= VALIDON;
    
    swap_pool[fr_index].sw_pte->pte_entryLO &= ~ENTRYLO_PFN_MASK;
    unsigned int pfn = (SWAP_POOL_START + (fr_index * PAGESIZE)) >> ENTRYLO_PFN_BIT;
    swap_pool[fr_index].sw_pte->pte_entryLO |= (pfn << ENTRYLO_PFN_BIT) & ENTRYLO_PFN_MASK;

    check_updateTLB(swap_pool[fr_index].sw_pte); // aggiorna TLB

    release_mutexTable(); // rilascio mutex
    LDST(state); // ripristina stato processo
}
