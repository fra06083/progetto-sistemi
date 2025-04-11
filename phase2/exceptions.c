#include "./headers/exceptions.h"
/*

Important: To determine if the Current Process was executing in kernel-mode or user-mode,
one examines the Status register in the saved exception state. In particular, examine the MPP field
if the status register with saved_exception_state->status & MSTATUS_MPP_MASK see section Dott.
Rovelli’s thesis for more details.

*/
void uTLB_ExceptionHandler(){


}
void programTrapHandler(){

}
void exceptionHandler()
{
  klog_print("Exception handler acceso\n");
    state_t *stato = GET_EXCEPTION_STATE_PTR(getPRID());
    int getExcepCode = stato->cause & CAUSE_EXCCODE_MASK;

    // Dobbiamo determinare se viene eseguito in Kernel-mode o User-mode
    // qui mandiamo l'eccezione al gestore delle interruzioni
    if (CAUSE_IS_INT(getExcepCode))
    {
        interruptHandler();
        return; // Esci dalla funzione dopo aver gestito l'interruzione
    }

    if (getExcepCode >= 24 && getExcepCode <= 28){
        /* TLB Exception Handler */
        uTLB_ExceptionHandler();
    } else if (getExcepCode >= 8 && getExcepCode <= 11) {
        /* Nucleus’s SYSCALL exception handler */
        syscallHandler(getExcepCode);
    } else {
        /* Nucleus’s Program Trap exception handler */
        programTrapHandler();
    }
}
void createProcess(state_t *c_state){
  ACQUIRE_LOCK(&global_lock);
  pcb_t *new_process = allocPcb();
  if (new_process == NULL) {
    c_state->reg_a0 = -1; // restituisco -1 nel registro a0 se non posso creare un processo
    RELEASE_LOCK(&global_lock);
  }
  // sbagliato new_process->p_s = c_state->reg_a1;
  new_process->p_supportStruct = (support_t *) c_state->reg_a3;
  insertProcQ(&current_process[getPRID()]->p_list, new_process);
  insertChild(current_process[getPRID()], new_process);
  process_count++;
  new_process->p_time = 0;
  new_process->p_semAdd = NULL;
  c_state->reg_a0 = new_process->p_pid;
  RELEASE_LOCK(&global_lock);
}
void terminateProcess(state_t *c_state, unsigned int p_id){
  // dobbiamo terminare il processo corrente
  ACQUIRE_LOCK(&global_lock);
  if (p_id==0){
    // terminiamo il processo
    removeProcQ(&pcbReady);
    process_count--;
    current_process[0] = NULL;
  } else {
   // GET_EXCEPTION_STATE_PTR(p_id); ?? tutor dice di usare questo
  }
  RELEASE_LOCK(&global_lock);
  
}
void syscallHandler(int excepCode){
  klog_print("sys handler acceso\n");

// QUI VERIFICHIAMO SE E' in kernel mode
/*
important: In the implementation of each syscall, remember to access the global variables Process
Count, Ready Queue and Device Semaphores in a mutually exclusive way using ACQUIRE_LOCK and
RELEASE_LOCK on the Global Lock, in order to avoid race condition.
*/
state_t *stato = GET_EXCEPTION_STATE_PTR(excepCode);
int iskernel = stato->status & MSTATUS_MPP_M;
unsigned int p_id = getPRID();
if (stato->reg_a0 < 0 && iskernel){
    // se il valore di a0 è negativo e siamo in kernel mode
    // allora dobbiamo fare un'azione descritta nella pagina 7 del pdf
   switch (stato->reg_a0)
   {
   case CREATEPROCESS:
    createProcess(stato);
    break;
   case TERMPROCESS:
    terminateProcess(stato, p_id);
   break;
   case PASSEREN:
  // Questo servizio richiede al Nucleus di eseguire un'operazione P su un semaforo binario. 
  // Il servizio P o NSYS3 viene richiesto dal processo chiamante posizionando il valore -3 in a0, 
  // l'indirizzo fisico del semaforo su cui eseguire l'operazione P è in a1, e poi dobbiamo eseguire la SYSCALL. 
  // A seconda del valore del semaforo, il controllo viene restituito al Processo Corrente alla CPU corrente, 
  // oppure questo processo viene bloccato sull'ASL (passa da "running" a "blocked") <= dobbiamo cambiarlo, direi di fare delle if e viene chiamato lo Scheduler.
    // prendiamo il semaforo ma come controlliamo se è 0 o 1?
    semd_t *semaforo = (semd_t *)stato->reg_a1;
    if (*semaforo->s_key > 1){
      // fa la p e continua il processo
      semaforo->s_key--;
    } else {
      // fa la p e lo mette in blocked
      // dobbiamo fare un'operazione di enqueue sul semaforo
      // e poi facciamo lo scheduler
      // se il semaforo è 0, allora dobbiamo bloccare il processo corrente
      // e fare lo scheduler
      insertBlocked(semaforo->s_key, current_process[p_id]);
      // process_count--; ??? non è più attivo
      current_process[p_id]->p_time = 0;
      current_process[p_id] = NULL;
      // impostiamo a blocked qui manca istruzione
      // scheduler() ???
    }
    break;
    case VERHOGEN:
    break;
   case DOIO:
    // Questo servizio richiede al Nucleo di eseguire un'operazione di I/O su un dispositivo.
    
    break;
   case GETTIME:
    break;
   case CLOCKWAIT:
    break;
  case GETSUPPORTPTR:
    break;
  case GETPROCESSID:
    break;
  default: // Se non trova nessuna, program TRAP
   stato->cause = PRIVINSTR;
   programTrapHandler();
   }
} else {
  stato->cause = PRIVINSTR; // perché non è possibile usare questi casi in usermode.
  programTrapHandler();
}
}


/*    
    }

    */



