#include "./headers/scheduler.h"
/* FUNZIONI ESTERNE DA USARE QUI */
extern int process_count;
extern struct pcb_t *current_process[NCPU];
extern removeProcQ(struct list_head *head);
extern struct list_head ready_queue;
extern int getPRID();
extern volatile int global_lock;
void scheduler(){
    /* Remove the PCB from the head of the Ready Queue and store the pointer to the PCB in the
Current Process field of the current CPU.
*/  
    if (!global_lock){
        ACQUIRE_LOCK(&global_lock);
    }
    // Controllo prima se la ready queue è vuota
    if (list_empty(&ready_queue)){
        if (process_count == 0){
        // Se la ready queue è vuota e non ci sono processi in esecuzione, allora HALT
        /* Nel file è richiesto di RILASCIARE IL LOCK, guarda il tip alla fine, poi fai un HALT */
        RELEASE_LOCK(&global_lock);
        HALT();
        } else {
        // 2. Load 5 milliseconds on the PLT [Section 7.2].
        /* . Perform a Load Processor State (LDST) on the processor state stored in PCB of the Current
Process (p_s) of the current CPU.
*/
        int prid = getPRID();
        struct pcb_t *pcb = removeProcQ(&ready_queue);
        current_process[prid] = pcb;
        LDST(&current_process[prid]->p_s);
        }
    } else {
        setMIE(MIE_ALL & ~MIE_MTIE_MASK);
        unsigned int status = getSTATUS();
        status |= MSTATUS_MIE_MASK;
        setSTATUS(status);
        RELEASE_LOCK(&global_lock);
        LDIT(TIMESLICE);
        *((memaddr *)TPR) = 1;
        RELEASE_LOCK(&global_lock);
        WAIT();

    // L'ultima parte non l'ho capita
    }
}
