#include "./headers/vmSupport.h"

#define getFlashAddr(asid) (DEV_REG_ADDR(IL_FLASH, asid-1))
#define writeFlash(asid, flashAddr, block)  FlashRW(asid, flashAddr, block, 0)
#define readFlash(asid, flashAddr, block)   FlashRW(asid, flashAddr, block, 1)
#define SWAP_POOL_START (RAMSTART + (64 * PAGESIZE) + (NCPU * PAGESIZE))

/*
- TLB exception handler (The Pager) [Section 4].
- function(s) for reading and writing flash devices
- the Swap Pool table is local to this module
*/


//POLITICA PER RIMPIAZZAMENTO FRAME SWAP POOL
int selectSwapFrame(){     
    static int next_frame_index = 0;// Page replacement FIFO (5.4) 
    for (int i = 0; i < POOLSIZE; i++) {
        if (swap_pool[i].sw_asid == -1) {
                return i; // frame libero
        }
    }
    return (next_frame_index++ % POOLSIZE); // "increment this variable mod the size of the Swap Pool"
    
}

void check_updateTLB(pteEntry_t *pte){
    setENTRYHI(pte->pte_entryHI);       //Metto l'entry della PT del processo corrente, guardo se è nella TLB
                                        //Comando che scrive in un Registro: EntryHiRegister
    TLBP();                             //TLBP cerca solo l'entryHi contenuta nel EntryHiRegister, non serve setEntryLO solo quando scrivo nella TLB (comando TLBWR)
                                         //comando per fare l'operazione di search nella TLB del processore, cerca se nella TLB c'è un match col valore di EntryHi register (corrente direi, dovrebbe essere come TLBWR sul processore corrente):
    unsigned int index = getINDEX();       //Ottengo il valore dell' Index Register
    //se probe bit è 1 non c'è match
    //se probe bit è 0 c'è match
    if(!(index & PRESENTFLAG)){           // negato sennò non entra
        //Caso no match is found
        //Mutua esclusione non serve perchè ogni processore ha la sua TLB
        //Istruzioni per scrivere nella TLB
        setENTRYLO(pte->pte_entryLO); // Imposta l'entry LO Register, entryHi già settato sopra
        TLBWI(); // Scrive l'entry nella TLB
    }//Se non va nell'if la TLB contiene già la entry
    
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
    int error = read ? 4 : 5; // 4: FLASHREAD ERROR, 5: FLASHWRITE ERROR
    if ((status & 0xFF) == error) { 
        release_mutexTable();
        supportTrapHandler(asid);
    }
}


void uTLB_ExceptionHandler() {  
    support_t *sup_ptr = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);       
    state_t *state = &(sup_ptr->sup_exceptState[PGFAULTEXCEPT]);             //Cause of the TLB Exception (4.2)  

    if (state->cause == EXC_MOD) { // punto 4.2
      supportTrapHandler(sup_ptr->sup_asid);
      return; // Non proseguire nel dispatch SYSCALL 
    }

    unsigned int p = getPageIndex(state->entry_hi);          // punto 5 (4.2 Pager)
    unsigned int ASID = sup_ptr->sup_asid;                   // ASID: asid current process
    acquire_mutexTable(ASID);                                // punto 4 (4.2 Pager)
                                                
    int found = FALSE;
    int i;    
    for (i = 0; i < POOLSIZE && !found; i++){
        unsigned int sw_asid = swap_pool[i].sw_asid;
        if (sw_asid == ASID && swap_pool[i].sw_pageNo == p){
            found = TRUE;
            break;
        }   
    }

    if (found) {  
        check_updateTLB(swap_pool[i].sw_pte);                           
        if (sup_ptr->sup_privatePgTbl[p].pte_entryLO & ENTRYLO_VALID) { // controlliamo se è valido, rilasciamo e ricarichiamo
            release_mutexTable();
            LDST(state);
        }
    } 
    // ... continua punto 7         
    // Seleziona un frame dalla Swap Pool usando l'algoritmo di page replacement 
    int fr_index = selectSwapFrame();    

    if(swap_pool[fr_index].sw_asid != -1 ){                        //asid -1 -> frame libero 
    //frame occupato, lo libero (punto 9)
        int k = swap_pool[fr_index].sw_pageNo;                     //Virtual Page Number k del frame occupato 
        int asid_proc_x = swap_pool[fr_index].sw_asid;             //k e processo x (asid) ~ stessi nomi documentazione 
        pteEntry_t *entry = swap_pool[fr_index].sw_pte;
        entry->pte_entryLO &= ~VALIDON;                             // spegne il vbit impostandolo ad invalid  
        //punto 9 (b): Update TLB if needed
        check_updateTLB(entry);
        writeFlash(asid_proc_x, SWAP_POOL_START + (fr_index * PAGESIZE), k); //Scrivo il frame nel flash device
    } 
    readFlash(ASID, SWAP_POOL_START + (fr_index * PAGESIZE), p);
    
    //Punto 10
    swap_pool[fr_index].sw_asid = ASID;                                  //Aggiorno la swap pool per dire che il frame i è occupato dal processo ASID
    swap_pool[fr_index].sw_pageNo = p;                                   //Aggiorno la swap pool per dire che il frame i contiene la pagina p
    swap_pool[fr_index].sw_pte = &(sup_ptr->sup_privatePgTbl[p]);
    swap_pool[fr_index].sw_pte->pte_entryLO |= VALIDON; // valida page p OR BIT
 
     
    swap_pool[fr_index].sw_pte->pte_entryLO &= ~ENTRYLO_PFN_MASK; 
    unsigned int pfn = (SWAP_POOL_START + (fr_index * PAGESIZE)) >> ENTRYLO_PFN_BIT;
    swap_pool[fr_index].sw_pte->pte_entryLO |= (pfn << ENTRYLO_PFN_BIT) & ENTRYLO_PFN_MASK;

    //Devo aggiornare il PFN field nella pte_entry_LO della pagina p del processo ASID
    //Aggiorno la TLB
    check_updateTLB(swap_pool[fr_index].sw_pte);

    //Punto 11
    release_mutexTable(); // Rilascio il mutex della swap pool
    LDST(state); // Ripristino lo stato del processo che ha causato il page fault     
}
