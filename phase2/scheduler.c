#include "./headers/scheduler.h"
void scheduler(){
    /* Remove the PCB from the head of the Ready Queue and store the pointer to the PCB in the
Current Process field of the current CPU.
*/
ACQUIRE_LOCK(&global_lock);  // Acquisiamo il lock

// Controllo se la ready queue è vuota
if (list_empty(&pcbReady)) {
    klog_print("Ready queue is empty.\n");
    if (process_count == 0) {
        // Ready queue vuota e nessun processo in esecuzione, quindi HALT
        RELEASE_LOCK(&global_lock);  // Rilascio del lock prima di HALT
        HALT(); // Fermiamo l'esecuzione
    } else {
        // Ready queue vuota, ma ci sono processi in esecuzione, quindi WAIT
        setMIE(MIE_ALL & ~MIE_MTIE_MASK);
        unsigned int status = getSTATUS();
        status |= MSTATUS_MIE_MASK;
        setSTATUS(status);
        LDIT(TIMESLICE);
        *((memaddr *)TPR) = 1;  // Settiamo il TPR a 1 prima di fare WAIT
        RELEASE_LOCK(&global_lock);
        WAIT();
    }
} else {
    klog_print("Dispatching next process...\n");
    // Dispatch del prossimo processo
    int processid = getPRID();
    klog_print("Preso id\n");
    struct pcb_t *pcb = removeProcQ(&pcbReady);
    klog_print("rimosso\n");
    current_process[processid] = pcb;
    klog_print("messso nel current process\n");
    
    // Settiamo il timeslice per il processo
    LDIT(TIMESLICE);
    setTIMER(TIMESLICE);  // Impostiamo il timer per il timeslice
    
    // Carichiamo il contesto del processo
    klog_print("Caricamento del contesto del processo...\n");
    // Rilasciamo il lock dopo aver completato il dispatch
    RELEASE_LOCK(&global_lock);
    klog_print("LOCK RILASCIATO\n");
    setTIMER(TIMESLICE);
    LDST(&(pcb->p_s));
    klog_print("Contesto caricato\n");
}
}
