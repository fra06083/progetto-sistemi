#include "./headers/scheduler.h"


void scheduler() {
    
  /* Remove the PCB from the head of the Ready Queue and store the pointer to the PCB in the
Current Process field of the current CPU.
*/
  ACQUIRE_LOCK(&global_lock);
  // Controllo se la ready queue è vuota
  if (emptyProcQ(&pcbReady)) {
    if (process_count == 0) {
      // Ready queue vuota e nessun processo in esecuzione, quindi HALT
      RELEASE_LOCK(&global_lock); // Rilascio del lock prima di HALT
      HALT(); // Fermiamo l'esecuzione
    } else {
      // Ready queue vuota, ma ci sono processi in esecuzione, quindi WAIT
      RELEASE_LOCK(&global_lock); // Rilascio del lock prima di WAIT
      setMIE(MIE_ALL & ~MIE_MTIE_MASK);
      unsigned int status = getSTATUS();
      status |= MSTATUS_MIE_MASK;
      setSTATUS(status);
      *((memaddr * ) TPR) = 1; // Settiamo il TPR a 1 prima di fare WAIT
      WAIT();
    }
  } else {
    // Dispatch del prossimo processo
    int pid = getPRID();
    pcb_t *pcb = removeProcQ(&pcbReady);
    current_process[pid] = pcb;
    // Settiamo il timeslice per il processo   // LDIT(TIMESLICE);
    setTIMER(TIMESLICE); // Impostiamo il timer per il timeslice
    *((memaddr*)TPR) = 0; // Settiamo il TPR a 0 per abilitare le interruzioni
    STCK(start_time[pid]);  // settiamo il tempo di inizio del processo
    // Rilasciamo il lock dopo aver completato il dispatch
    RELEASE_LOCK(&global_lock);
    // Carichiamo il contesto del processo
    LDST(&(pcb->p_s));
  }
}