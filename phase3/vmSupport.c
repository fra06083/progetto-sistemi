#include "./headers/vmSupport.h"
#define getFlashAddr(asid) (DEV_REG_ADDR(IL_FLASH, asid-1))
#define writeFlash(asid, flashAddr, block)  FlashRW(asid, flashAddr, block, 0)
#define readFlash(asid, flashAddr, block)   FlashRW(asid, flashAddr, block, 1)
extern void klog_print(const char *msg);
/*
- TLB exception handler (The Pager) [Section 4].
- function(s) for reading and writing flash devices
- the Swap Pool table is local to this module
*/



int selectSwapFrame(){                                    // Page replacement FIFO (5.4)
    int selected_frame = next_frame_index;
    next_frame_index++;
    next_frame_index = (next_frame_index) % POOLSIZE; // "increment this variable mod the size of the Swap Pool"
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

void updateTLB(pteEntry_t *pte) {
    unsigned int entryHI = pte->pte_entryHI;
    unsigned int entryLO = pte->pte_entryLO;
    //Mutua esclusione non serve perchè ogni processore ha la sua TLB
    //Istruzioni per scrivere nella TLB
    setENTRYHI(entryHI); // Imposta l'entry HI
    setENTRYLO(entryLO); // Imposta l'entry LO
    TLBWR(); // Scrive l'entry nella TLB
}

   
void FlashRW(int asid, memaddr frameAddr, int block, int read){
 //Punto 9 (c)
 //Block number = VPN k (corrispondono)
 //Scrivere nel DATA0 field (del flash device) l'indirizzo fisico di partenza di un certo frame
 int semIndex = findDevice((memaddr*) getFlashAddr(asid));
 acquireDevice(asid, semIndex);
 dtpreg_t *devreg = (dtpreg_t *) getFlashAddr(asid); // mi dà l'indirizzo del registro del flash
 int commandAddr = (int)&devreg->command;
 int commandValue = (read ? FLASHREAD : FLASHWRITE) | (block << 8); // qua il comando cambia a seconda che sia read o write
 devreg->data0 = frameAddr;
 int status = SYSCALL(DOIO, commandAddr, commandValue, 0);
 releaseDevice(asid, semIndex);
 int error = read ? 4 : 5; // 4: FLASHREAD_ERROR, 5: FLASHWRITE_ERROR
    if ((status & 0XFF) == error) { // treat flash I/O error as a program trap
        release_mutexTable();
        supportTrapHandler(asid);
    }
}
void uTLB_ExceptionHandler() {  
    support_t *sup_ptr = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);       //NSYS8 (pulire commenti, uso come placeholder)
    state_t *state = &(sup_ptr->sup_exceptState[PGFAULTEXCEPT]);             //Cause of the TLB Exception (4.2)
    if (state->cause == EXC_MOD) { // punto 4.2
      supportTrapHandler(sup_ptr->sup_asid);
      return; // Non proseguire nel dispatch SYSCALL 
    }
    unsigned int p = getPageIndex(state->entry_hi);                          // punto 5 (4.2 Pager)
    unsigned int ASID = sup_ptr->sup_asid;                                   // ASID: asid current process
    acquire_mutexTable(ASID);                                                // punto 4 (4.2 Pager)
    int trovato = FALSE;
    int i; 
    for (i = 0; i < POOLSIZE && !trovato; i++){
        unsigned int sw_asid = swap_pool[i].sw_asid;
        if (sw_asid == ASID && swap_pool[i].sw_pageNo == p)
            trovato = TRUE;
    }
    if (trovato) {
        klog_print("trovato\n");
        updateTLB(swap_pool[i].sw_pte);
        if (sup_ptr->sup_privatePgTbl[p].pte_entryLO & ENTRYLO_VALID) { // controlliamo se è valido, rilasciamo e ricarichiamo
            release_mutexTable();
            LDST(state);
        } 
    }
    klog_print("non trovato\n");
    // ... continua punto 7         
    // Seleziona un frame dalla Swap Pool usando l'algoritmo di page replacement
    unsigned int fr_index = selectSwapFrame();     //FCFS() recommended/RR();
    if(swap_pool[fr_index].sw_asid != -1 ){                         //asid -1 -> frame libero 
    //frame occupato, lo libero (punto 9)
        int k = swap_pool[fr_index].sw_pageNo;                     //Virtual Page Number k del frame occupato 
        int asid_proc_x = swap_pool[fr_index].sw_asid;             //k e processo x (asid) ~ stessi nomi documentazione 
        pteEntry_t *entry = swap_pool[fr_index].sw_pte;
        entry->pte_entryLO &= ~VALIDON; // spegne il vbit impostandolo ad invalid  
        //punto 9 (b): Update TLB if needed
        updateTLB(entry);
        writeFlash(asid_proc_x, FRAMEPOOLSTART + (fr_index * PAGESIZE), k); //Scrivo il frame nel flash device
    /*    int trovato = FALSE;             
        for (int i = 0; i < POOLSIZE && !trovato; i++){             //Da telegram: swap_mutex così l'esecuzione è atomica 9 (a,b), manca disabilitazione interrupt in questa fase 
            unsigned int sw_asid = swap_pool[i].sw_asid;
            if (sw_asid == ASID && swap_pool[i].sw_pageNo == k)
                trovato = TRUE;
            } 
            if(trovato){
                updateTLB(swap_pool[fr_index].sw_pte);
                writeFlash(asid_proc_x, FRAMEPOOLSTART + (fr_index * PAGESIZE), k); //Scrivo il frame nel flash device
            }
        */
    } 
    readFlash(ASID, FRAMEPOOLSTART + (fr_index * PAGESIZE), p);
    //Punto 10
    swap_pool[fr_index].sw_asid = ASID;                                  //Aggiorno la swap pool per dire che il frame i è occupato dal processo ASID
    swap_pool[fr_index].sw_pageNo = p;                                   //Aggiorno la swap pool per dire che il frame i contiene la pagina p
    swap_pool[fr_index].sw_pte = &(sup_ptr->sup_privatePgTbl[p]);
    swap_pool[fr_index].sw_pte->pte_entryLO |= VALIDON; // valida page p OR BIT
    swap_pool[fr_index].sw_pte->pte_entryLO &= ~ENTRYLO_PFN_MASK; // imposta pfn 0 // AND tra quello esistente e negato
    swap_pool[fr_index].sw_pte->pte_entryLO |= (FRAMEPOOLSTART + (fr_index * PAGESIZE)); // imposta la pfn all'indirizzo del frame
    //Devo aggiornare il PFN field nella pte_entry_LO della pagina p del processo ASID
    //Aggiorno la TLB
    updateTLB(swap_pool[fr_index].sw_pte);
    //Punto 11
    release_mutexTable(); // Rilascio il mutex della swap pool
    LDST(state); // Ripristino lo stato del processo che ha causato il page fault     
}
