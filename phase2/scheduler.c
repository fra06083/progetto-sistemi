#include "./headers/scheduler.h"
void scheduler(){
    /* Remove the PCB from the head of the Ready Queue and store the pointer to the PCB in the
Current Process field of the current CPU.
*/
    if (!global_lock){
        ACQUIRE_LOCK(&global_lock);
    }
    // Controllo prima se la ready queue è vuota
    if (list_empty(&pcbReady)){
        if (process_count == 0){
            // Se la ready queue è vuota e non ci sono processi in esecuzione, allora HALT
            /* Nel file è richiesto di RILASCIARE IL LOCK, guarda il tip alla fine, poi fai un HALT */
            RELEASE_LOCK(&global_lock);
            HALT();
        } else {
            // Se la ready queue è vuota ma ci sono processi in esecuzione, allora WAIT
            setMIE(MIE_ALL & ~MIE_MTIE_MASK);
            unsigned int status = getSTATUS();
            status |= MSTATUS_MIE_MASK;
            setSTATUS(status);
            LDIT(TIMESLICE);
            *((memaddr *)TPR) = 1;
            RELEASE_LOCK(&global_lock);
            WAIT();
        }
    } else {
        /* prendiamo l'id del processore corrente, 
        rimuoviamo il pcb dalla ready queue e lo mettiamo nel current process; (ARRAY di puntatori a pcb)
        poi facciamo il dispatch */
        int prid = getPRID();
        struct pcb_t *pcb = removeProcQ(&pcbReady);
        current_process[prid] = pcb;
        LDIT(TIMESLICE);
        LDST(&current_process[prid]->p_s);
        RELEASE_LOCK(&global_lock);
    }
}
