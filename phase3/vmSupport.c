#include "./headers/vmSupport.h"
#define getFlashAddr(asid) (DEV_REG_ADDR(IL_FLASH, asid-1))
#define writeFlash(asid, flashAddr, block)  FlashRW(asid, flashAddr, block, 0)
#define readFlash(asid, flashAddr, block)   FlashRW(asid, flashAddr, block, 1)
#define SWAP_POOL_START (RAMSTART + (64 * PAGESIZE) + (NCPU * PAGESIZE))
static int next_frame_index = 0; // Static x FIFO round-robin
extern void klog_print(const char *msg);
/*
- TLB exception handler (The Pager) [Section 4].
- function(s) for reading and writing flash devices
- the Swap Pool table is local to this module
*/

int selectSwapFrame(){                                    // Page replacement FIFO (5.4)
    klog_print("Frame selezionato\n"); 
    for (int i = 0; i < POOLSIZE; i++) {
        if (swap_pool[i].sw_asid == -1) {
                return i;
        }
    }
    next_frame_index = (next_frame_index+1) % POOLSIZE; // "increment this variable mod the size of the Swap Pool"
    return next_frame_index;
}

void acquire_mutexTable(int asid){
    SYSCALL(PASSEREN, (int)&swap_mutex, 0, 0);
    asidAcquired = asid;
}
void release_mutexTable(){
    SYSCALL(VERHOGEN, (int)&swap_mutex, 0, 0);
    asidAcquired = -1;
}

void check_updateTLB(pteEntry_t *pte){
    //Metto l'entry della page Table del processo corrente, guardo se è nella TLB
    setENTRYHI(pte->pte_entryHI);       //Comando che scrive in un Registro: EntryHiRegister
    //TLBP cerca solo l'entryHi contenuta nel EntryHiRegister, non serve setEntryLO solo quando scrivo nella TLB (comando TLBWR)
    TLBP();         //comando per fare l'operazione di search nella TLB del processore, cerca se nella TLB c'è un match col valore di EntryHi register (corrente direi, dovrebbe essere come TLBWR sul processore corrente):
    unsigned int index = getINDEX();        //Ottengo il valore dell' Index Register
    if(!(index & PRESENTFLAG)){           //Leggo il Probe bit, 1 ahia , 0 tutt'apposto, però è da negare sennò non entra 
        //Caso no match is found
        //Mutua esclusione non serve perchè ogni processore ha la sua TLB
        //Istruzioni per scrivere nella TLB
        setENTRYLO(pte->pte_entryLO); // Imposta l'entry LO Register, entryHi già settato sopra
        TLBWR(); // Scrive l'entry nella TLB
    }//Se non va nell'if la TLB contiene già la entry
}


extern void klog_print_dec(unsigned int num);  
void FlashRW(int asid, memaddr frameAddr, int block, int read){
    print("flashRW \n");
    //Punto 9 (c)
    //Block number = VPN k (corrispondono)
    //Scrivere nel DATA0 field (del flash device) l'indirizzo fisico di partenza di un certo frame
    int semIndex = findDevice((memaddr*) getFlashAddr(asid));
    klog_print_dec(semIndex);
    acquireDevice(asid, semIndex);
    dtpreg_t *devreg = (dtpreg_t *) getFlashAddr(asid); // mi dà l'indirizzo del registro del flash
    int commandAddr = (int)&devreg->command;
    int commandValue = (read ? FLASHREAD : FLASHWRITE) | (block << 8); // qua il comando cambia a seconda che sia read o write
    devreg->data0 = frameAddr;
    int status = SYSCALL(DOIO, commandAddr, commandValue, 0);
    releaseDevice(asid, semIndex);
    int error = read ? 4 : 5; // 4: FLASHREAD_ERROR, 5: FLASHWRITE_ERROR
    if ((status & 0XFF) == error) { 
        release_mutexTable();
        supportTrapHandler(asid);
    }
}
void uTLB_ExceptionHandler() {
    print("uTLB EXCEPTION\n");  
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
        check_updateTLB(swap_pool[i].sw_pte);                           
        if (sup_ptr->sup_privatePgTbl[p].pte_entryLO & ENTRYLO_VALID) { // controlliamo se è valido, rilasciamo e ricarichiamo
            release_mutexTable();
            LDST(state);
        } 
    }  
    // ... continua punto 7         
    // Seleziona un frame dalla Swap Pool usando l'algoritmo di page replacement 
    int fr_index = selectSwapFrame();     //FCFS() recommended/RR();
    if(swap_pool[fr_index].sw_asid != -1 ){                         //asid -1 -> frame libero 
    //frame occupato, lo libero (punto 9)
        print("frame occupato\n");
        int k = swap_pool[fr_index].sw_pageNo;                     //Virtual Page Number k del frame occupato 
        int asid_proc_x = swap_pool[fr_index].sw_asid;             //k e processo x (asid) ~ stessi nomi documentazione 
        pteEntry_t *entry = swap_pool[fr_index].sw_pte;
        entry->pte_entryLO &= ~VALIDON; // spegne il vbit impostandolo ad invalid  
        //punto 9 (b): Update TLB if needed
        check_updateTLB(entry);
        writeFlash(asid_proc_x, SWAP_POOL_START + (fr_index * PAGESIZE), k); //Scrivo il frame nel flash device
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
    print("frame non occupato\n");
    readFlash(ASID, SWAP_POOL_START + (fr_index * PAGESIZE), p);
    //Punto 10
    swap_pool[fr_index].sw_asid = ASID;                                  //Aggiorno la swap pool per dire che il frame i è occupato dal processo ASID
    swap_pool[fr_index].sw_pageNo = p;                                   //Aggiorno la swap pool per dire che il frame i contiene la pagina p
    swap_pool[fr_index].sw_pte = &(sup_ptr->sup_privatePgTbl[p]);
    swap_pool[fr_index].sw_pte->pte_entryLO |= VALIDON; // valida page p OR BIT
    swap_pool[fr_index].sw_pte->pte_entryLO &= ~ENTRYLO_PFN_MASK; // imposta pfn 0 // AND tra quello esistente e negato
    swap_pool[fr_index].sw_pte->pte_entryLO |= (SWAP_POOL_START + (fr_index * PAGESIZE)); // imposta la pfn all'indirizzo del frame
    //Devo aggiornare il PFN field nella pte_entry_LO della pagina p del processo ASID
    //Aggiorno la TLB
    check_updateTLB(swap_pool[fr_index].sw_pte);
    //Punto 11
    release_mutexTable(); // Rilascio il mutex della swap pool
    LDST(state); // Ripristino lo stato del processo che ha causato il page fault     
}
