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
    state_t *stato = GET_EXCEPTION_STATE_PTR(getPRID());
    int getExcepCode = getCAUSE() & CAUSE_EXCCODE_MASK;
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
        syscallHandler(stato);
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
  c_state->pc_epc += 4; // lo dice nel punto dopo
  LDST(c_state);
}
void terminateProcess(state_t *c_state, unsigned int p_id){
  // dobbiamo terminare il processo corrente
  ACQUIRE_LOCK(&global_lock);
  if (p_id==0){
    // terminiamo il processo
    killProcess(current_process[getPRID()]);
    current_process[getPRID()] = NULL;
    process_count--;
    RELEASE_LOCK(&global_lock);
    scheduler(); // va nello scheduler se abbiamo terminato il processo corrente
  } else { 
    // CERCHIAMO il processo, mettiamo true così lo rimuove di già
  pcb_t *process = findProcess(p_id, 1);
  if (process != NULL){
    killProcess(process); // non lo abbiamo trovato, il termprocess vuole terminare un processo che non esiste
    // dovremo far ripartire l'esecuzione del processo ma come??
    scheduler();
  }
  RELEASE_LOCK(&global_lock);
  // dovremo far ripartire l'esecuzione del processo ma come??
  c_state->pc_epc += 4;             // increment the program counter
  LDST(c_state);
  }
}

void P(state_t* stato, unsigned int p_id){
    klog_print(" Syscall handler PASSEREN() ");
    int *semaforo = (int *) stato->reg_a1;
    (*semaforo)--;
    if (*semaforo >= 1){
      klog_print(" P() if ");
      ACQUIRE_LOCK(&global_lock);
      pcb_t* processo_sbloccato = removeBlocked(semaforo);  // Controlliamo se c'è un processo bloccato
      if (processo_sbloccato != NULL) {
        insertProcQ(&pcbReady, processo_sbloccato); 
      }
    } else {
      klog_print(" P() else ");
      // fa la p e lo mette in blocked
      // dobbiamo fare un'operazione di enqueue sul semaforo
      // e poi facciamo lo scheduler
      // se il semaforo è 0, allora dobbiamo bloccare il processo corrente
      // e fare lo scheduler
      ACQUIRE_LOCK(&global_lock);
      pcb_t* pcbBlocked = current_process[p_id];
      insertBlocked(semaforo, pcbBlocked);
     // process_count--; devo farlo?
      stato->pc_epc += 4; // incrementiamo il program counter
      // incrementiamo il program counter lo dice nella sezione 6.12, previene il syscall loop
      // dobbiamo gestire anche il tempo TODO
      pcbBlocked->p_s = *stato; // salviamo lo stato del processo
      pcbBlocked->p_time = 0;
      current_process[p_id] = NULL;
      RELEASE_LOCK(&global_lock);
      scheduler();
      return;
    }
    RELEASE_LOCK(&global_lock);
    stato->pc_epc += 4; // incrementiamo il program counter
    LDST(stato);
}

void V(state_t* stato, unsigned int p_id){
  klog_print("SyscallHandler: VERHOGEN");
   int *semaforo = (int *) stato->reg_a1;
   (*semaforo)++;
  if (*semaforo <= 0){  //   PREV - RIGA 
 // if(!emptyProcQ(&semaforo->s_procq)){
    klog_print(" V() if ");
    // fa la p e continua il processo
    ACQUIRE_LOCK(&global_lock);
    pcb_t *processo_sbloccato = removeBlocked(semaforo);
    if (processo_sbloccato != NULL){
      // mettiamo il processo in ready
      insertProcQ(&pcbReady, processo_sbloccato);
    }
  } else if (*semaforo > 1) {
    // LA V in questo caso è bloccante
    klog_print(" V() else ");
    ACQUIRE_LOCK(&global_lock);
    pcb_t* pcbBlocked = current_process[p_id];
    stato->pc_epc += 4;
    // incrementiamo il program counter lo dice nella sezione 6.12, previene il syscall loop
    // dobbiamo gestire anche il tempo TODO
    pcbBlocked->p_s = *stato;
    insertBlocked(semaforo, pcbBlocked);
   // pcbBlocked->p_time = 0; || dobbiamo ricaricare il tempo di esec. non è così
    current_process[p_id] = NULL;
    RELEASE_LOCK(&global_lock);
    scheduler();
    return;
  }
  RELEASE_LOCK(&global_lock);
  stato->pc_epc += 4; // incrementiamo il program counter
  LDST(stato); //  PREV - RIGA Qui facciamo ripartire il processo
}

void syscallHandler(state_t *stato){

// QUI VERIFICHIAMO SE E' in kernel mode
/*
important: In the implementation of each syscall, remember to access the global variables Process
Count, Ready Queue and Device Semaphores in a mutually exclusive way using ACQUIRE_LOCK and
RELEASE_LOCK on the Global Lock, in order to avoid race condition.
*/
int iskernel = stato->status & MSTATUS_MPP_M;
unsigned int p_id = getPRID();
int registro = stato->reg_a0;
if (registro < 0 && iskernel){
    // se il valore di a0 è negativo e siamo in kernel mode
    // allora dobbiamo fare un'azione descritta nella pagina 7 del pdf
   switch (registro)
   {
   case CREATEPROCESS:
    createProcess(stato);
    break;
   case TERMPROCESS:
    terminateProcess(stato, stato->reg_a1);
   break;
   case PASSEREN:
    P(stato, p_id);
    break;
    case VERHOGEN:
    V(stato, p_id);
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
  klog_print("SyscallHandler > 0");
  stato->cause = PRIVINSTR; // perché non è possibile usare questi casi in usermode.
  programTrapHandler();
}
}


/*    
    }

    */



