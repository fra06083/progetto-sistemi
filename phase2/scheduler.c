#include "./headers/scheduler.h"


void scheduler() {

  /* Remove the PCB from the head of the Ready Queue and store the pointer to the PCB in the
Current Process field of the current CPU.
*/
  if (getPRID() != 0 || !lock_cpu0) {
    ACQUIRE_LOCK( & global_lock); // Blocchiamo le altre cpu
  }
  // Controllo se la ready queue è vuota
  if (emptyProcQ( & pcbReady)) {
    if (process_count == 0) {
      // Ready queue vuota e nessun processo in esecuzione, quindi HALT
      RELEASE_LOCK( & global_lock); // Rilascio del lock prima di HALT
      HALT(); // Fermiamo l'esecuzione
    } else {
      // Ready queue vuota, ma ci sono processi in esecuzione, quindi WAIT
      setMIE(MIE_ALL & ~MIE_MTIE_MASK);
      unsigned int status = getSTATUS();
      status |= MSTATUS_MIE_MASK;
      setSTATUS(status);
      *((memaddr * ) TPR) = 1; // Settiamo il TPR a 1 prima di fare WAIT
      RELEASE_LOCK(&global_lock);
      WAIT();
    }
  } else {
    klog_print("Dispatching next process...\n");
    // Dispatch del prossimo processo
    int pid = getPRID();
    klog_print("Preso id\n");
    struct pcb_t * pcb = removeProcQ( & pcbReady);
    if (pcb == NULL) {
      klog_print("Error: PCB is NULL\n");
    }
    klog_print("rimosso\n");
    current_process[pid] = pcb;
    if (lock_cpu0 && pid == 0)
      lock_cpu0 = 0;
    // Settiamo il timeslice per il processo
    LDIT(TIMESLICE);
    setTIMER(TIMESLICE); // Impostiamo il timer per il timeslice
    // Rilasciamo il lock dopo aver completato il dispatch
    RELEASE_LOCK( & global_lock);
    setTIMER(TIMESLICE);
    // Carichiamo il contesto del processo
    LDST(&(pcb -> p_s));
  }
}