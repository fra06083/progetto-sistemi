#include "./headers/vmSupport.h"
/*
- TLB exception handler (The Pager) [Section 4].
- function(s) for reading and writing flash devices
- the Swap Pool table is local to this module
*/



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

void updateTLB(pteEntry_t *pte) {
    unsigned int entryHI = pte->pte_entryHI;
    unsigned int entryLO = pte->pte_entryLO;
    //Mutua esclusione non serve perchè ogni processore ha la sua TLB
    //Istruzioni per scrivere nella TLB
    setENTRYHI(entryHI); // Imposta l'entry HI
    setENTRYLO(entryLO); // Imposta l'entry LO
    TLBWR(); // Scrive l'entry nella TLB
}

//void supportTrapHandler(support_t sup_ptr) già dichiarato in sysSupport 
   



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
    int i; 
    for (i = 0; i < POOLSIZE && !trovato; i++){
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
            //Block number = VPN k (corrispondono)
            //Scrivere nel DATA0 field (del flash device) l'indirizzo fisico di partenza di un certo frame 
            memaddr fr_addr = FRAMEPOOLSTART + (fr_index * PAGESIZE);                 //Sono impazzito a trovare la costante FRAMEPOOLSTART che non è MENZIONATA da nessuna parte (se non funziona comunque vuol dire che i frame partono nella RAM fisica dall'indirizzo 0x2002000)
            dtpreg_t *flash_dev_x = (dtpreg_t *) DEV_REG_ADDR(IL_FLASH, asid_proc_x - 1);                      //RIVEDERE !!!!!!
            flash_dev_x->data0 = fr_addr; 
            //int ioStatus = SYSCALL(DOIO, int *commandAddr, int commandValue, 0);
            int cmdVal = (k << 8) | FLASHWRITE;   
            int ioStatus = SYSCALL(DOIO, (int*) flash_dev_x->command,(int) cmdVal, 0);       //flash_dev_x + 0x8 è il command field address
            if(ioStatus != 1){    //Causa una TRAP se il comando non è andato a buon fine
                //supportTrapHandler(sup_ptr); 
            } 
            //Read the Current Process content of logical page p (in pratica devi fare una read sul flash device) into frame i (fr_index e fr_address)
            dtpreg_t *flash_dev_cp = (dtpreg_t *) DEV_REG_ADDR(IL_FLASH, ASID - 1); 
            flash_dev_cp->data0 = fr_addr;
            int commdVal = (p << 8) | FLASHREAD;
            int ioStatus_2 = SYSCALL(DOIO, (int*) flash_dev_cp->command, (int) commdVal, 0);      //flash_dev_cp + 0x8 è il command field address
            if(ioStatus_2 != 1){    //Causa una TRAP se il comando non è andato a buon fine (1 vedi uMPS3 doc)
                //supportTrapHandler(sup_ptr); 
            } 
            //Punto 10
            swap_pool[fr_index].sw_asid = ASID;                                  //Aggiorno la swap pool per dire che il frame i è occupato dal processo ASID
            swap_pool[fr_index].sw_pageNo = p;                                   //Aggiorno la swap pool per dire che il frame i contiene la pagina p
            swap_pool[fr_index].sw_pte = &(sup_ptr->sup_privatePgTbl[p]); 
            swap_pool[fr_index].sw_pte->pte_entryHI = (ASID << VBit) | p;       //Accendo il Vbit     
            //Devo aggiornare il PFN field nella pte_entry_LO della pagina p del processo ASID
            sup_ptr->sup_privatePgTbl[p].pte_entryLO = (fr_index << PAGESIZE) | VBit; //Aggiorno la pte_entry_LO della pagina p del processo ASID      
            //Aggiorno la TLB
            updateTLB(&sup_ptr->sup_privatePgTbl[p]);
            //Punto 11
            release_mutexTable(); // Rilascio il mutex della swap pool
            LDST(state); // Ripristino lo stato del processo che ha causato il page fault     
        }
}
