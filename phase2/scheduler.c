#include "./headers/scheduler.h"
void scheduler(){
    /* Remove the PCB from the head of the Ready Queue and store the pointer to the PCB in the
Current Process field of the current CPU.
*/
    if (!global_lock){ // Se il lock è 0, allora acquisiamo il lock
        ACQUIRE_LOCK(&global_lock); 
    } 
    // Controllo prima se la ready queue è vuota
    if (list_empty(&pcbReady)){
        ACQUIRE_LOCK(&global_lock);
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
            *((memaddr *)TPR) = 1; // Settiamo il TPR a 1 prima che faccia il wait
            RELEASE_LOCK(&global_lock);
            WAIT();
        }
    } else {
        /* prendiamo l'id del processore corrente, 
        rimuoviamo il pcb dalla ready queue e lo mettiamo nel current process; (ARRAY di puntatori a pcb)
        poi facciamo il dispatch */
        ACQUIRE_LOCK(&global_lock);
        int processid = getPRID();
        struct pcb_t *pcb = removeProcQ(&pcbReady);
        current_process[processid] = pcb;
        process_count--;
        LDIT(TIMESLICE);
        LDST(&current_process[processid]->p_s);
        // Il tutor ha scritto che devo caricare 50000ms con setTimer
        setTIMER(TIMESLICE);
        RELEASE_LOCK(&global_lock);
    }
}
