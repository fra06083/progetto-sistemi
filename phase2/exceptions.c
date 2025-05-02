#include "./headers/exceptions.h"

/*

Important: To determine if the Current Process was executing in kernel-mode or user-mode,
one examines the Status register in the saved exception state. In particular, examine the MPP field
if the status register with saved_exception_state->status & MSTATUS_MPP_MASK see section Dott.
Rovelli’s thesis for more details.

*/
void uTLB_ExceptionHandler(){


}
void programTrapHandler(int cause, state_t* stato){
  ACQUIRE_LOCK(&global_lock);
  int cpu_id = getPRID();
  pcb_t* current = current_process[cpu_id];

  if (current == NULL) {
    RELEASE_LOCK(&global_lock);
    scheduler();
  }

  if (current->p_supportStruct == NULL) {
    RELEASE_LOCK(&global_lock);
    killProcess(current);
    scheduler();
  }

  // Copia dell'eccezione
  current->p_supportStruct->sup_exceptState[cause] = *stato;
  context_t* context = &caller->p_supportStruct->sup_exceptContext[cause];
  LDCXT(context->stackPtr, context->status, context->pc);
  RELEASE_LOCK(&global_lock);
/*
  This function allows a current process to change its operating mode,
 * turning on/off interrupt masks, turning on user mode, and at the same time
 * changing the location of execution.
 * It is available only in kernel mode, thru a BIOS routine
 * (otherwise it causes a break).
 * It updates processor status, PC and stack pointer _completely_,
 * in one atomic operation.
 * It has no meaningful return value.
 * */
}
void exceptionHandler()
{
    state_t *stato = GET_EXCEPTION_STATE_PTR(getPRID());
    int getExcepCode = getCAUSE() & CAUSE_EXCCODE_MASK;
    // Dobbiamo determinare se viene eseguito in Kernel-mode o User-mode
    // qui mandiamo l'eccezione al gestore delle interruzioni
    // codici eccezioni in cpu.h
    if (CAUSE_IS_INT(getCAUSE())) {
      interruptHandler(getExcepCode, stato);  // if the cause is an interrupt, call the interrupt handler
    } else if (getExcepCode == EXC_ECU || getExcepCode == EXC_ECM) {
      syscallHandler(stato);
    } else if (getExcepCode >= EXC_MOD && getExcepCode <= EXC_UTLBS) {
      programTrapHandler(PGFAULTEXCEPT, stato);
    } else {
      programTrapHandler(GENERALEXCEPT, stato);
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

void P(state_t* stato, unsigned int p_id) {
  klog_print(" Syscall handler PASSEREN() ");
  ACQUIRE_LOCK(&global_lock); // acquisisci il lock prima di fare la P

  int *semaforo = (int *) stato->reg_a1;
  (*semaforo)--;  // decrementa il semaforo

  if (*semaforo < 0) {
      klog_print(" P() - blocco del processo chiamante ");
      // Blocca il processo corrente
      pcb_t* pcbBlocked = current_process[p_id];
      insertBlocked(semaforo, pcbBlocked);  // inserisci il processo nella lista dei bloccati

      stato->pc_epc += 4;  // incrementa il program counter
      // Salva lo stato del processo
      cpu_t tempo_fine = 0;
      STCK(tempo_fine);  // salva il tempo di fine
      pcbBlocked->p_time += tempo_fine - start_time[p_id];  // aggiorna il tempo di esecuzione del processo
      pcbBlocked->p_s = *stato;  // salva lo stato del processo
      current_process[p_id] = NULL;  // rimuovi il processo corrente dalla lista dei processi attivi

      RELEASE_LOCK(&global_lock);  // rilascia il lock
      scheduler();  // esegui lo scheduler per un altro processo
      return;  // ritorna e non continuare l'esecuzione del processo corrente
  } else {
      klog_print(" P() - continua l'esecuzione ");
      // Se il semaforo è >= 0, non bloccare e continua
      if (*semaforo >= 1) {
          pcb_t* processo_sbloccato = removeBlocked(semaforo);  // sblocca il processo in attesa
          if (processo_sbloccato != NULL) {
              insertProcQ(&pcbReady, processo_sbloccato);  // metti il processo nella coda dei ready
          }
      }
  }

  RELEASE_LOCK(&global_lock);  // rilascia il lock
  stato->pc_epc += 4;  // incrementa il program counter
  LDST(stato);  // ripristina lo stato del processo
}

void V(state_t* stato, unsigned int p_id) {
  klog_print(" Syscall handler VERHOGEN() ");
  ACQUIRE_LOCK(&global_lock);  // acquisisci il lock prima di fare la V

  int *semaforo = (int *) stato->reg_a1;
  (*semaforo)++;  // incrementa il semaforo

  if (*semaforo <= 0) {
      klog_print(" V() - sblocca un processo ");
      // Sblocca il processo in attesa
      pcb_t* processo_sbloccato = removeBlocked(semaforo);
      if (processo_sbloccato != NULL) {
          insertProcQ(&pcbReady, processo_sbloccato);  // metti il processo nella coda dei ready
      }
  } else if (*semaforo > 1) {
      klog_print(" V() - blocca il processo chiamante ");
      // Se il semaforo è maggiore di 1, blocca il processo chiamante (V bloccante)
      pcb_t* pcbBlocked = current_process[p_id];
      stato->pc_epc += 4;  // incrementa il program counter
      // Salva lo stato del processo
      cpu_t tempo_fine = 0;
      STCK(tempo_fine);  // salva il tempo di fine
      pcbBlocked->p_time += tempo_fine - start_time[p_id];  // aggiorna il tempo di esecuzione del processo
      pcbBlocked->p_s = *stato;  // salva lo stato del processo
      insertBlocked(semaforo, pcbBlocked);  // metti il processo nella lista dei bloccati

      current_process[p_id] = NULL;  // rimuovi il processo corrente dalla lista dei processi attivi

      RELEASE_LOCK(&global_lock);  // rilascia il lock
      scheduler();  // esegui lo scheduler per un altro processo
      return;  // ritorna e non continuare l'esecuzione del processo corrente
  }

  RELEASE_LOCK(&global_lock);  // rilascia il lock
  stato->pc_epc += 4;  // incrementa il program counter
  LDST(stato);  // ripristina lo stato del processo
}

void DoIO(state_t *stato, unsigned int p_id){
  // Questo servizio richiede al Nucleo di eseguire un'operazione di I/O su un dispositivo.
  // Dobbiamo implementare il codice per gestire questa operazione
  klog_print("SyscallHandler > DoIO");
  ACQUIRE_LOCK(&global_lock);
  memaddr* indirizzo_comando = (memaddr*) stato->reg_a1;  // prendiamo il comando e il suo valore
  int value = stato->reg_a2;                

  if (indirizzo_comando == NULL) {
    RELEASE_LOCK(&global_lock);
    stato->reg_a0 = -1;  // ritorna -1 se nullo
    stato->pc_epc += 4;  // incrementa il program counter
    LDST(stato);
    return;
  }

  pcb_t* pcb_attuale = current_process[p_id];       
  int devIndex = findDevice(indirizzo_comando - 1);  // get the device index from the command address
  
  if (devIndex < 0) {
    RELEASE_LOCK(&global_lock);
    stato->reg_a0 = -1;  // if the device index is not valid, return -1
    stato->pc_epc += 4;  // increment the program counter
    LDST(stato);
    return;
  }
  /* P semplificata in i/o */
  int *semPTR = &sem[devIndex];  // get the semaphore for the device
  sem[devIndex]--;   // decrementa il semaforo per bloccare il processo fino a quando l'operazione input/output è completata
  stato->pc_epc += 4; 
  pcb_attuale->p_s = *stato;
  insertBlocked(semPTR, pcb_attuale);  // inseisci il processo nei bloccati
  current_process[p_id] = NULL;
  cpu_t tempo_fine = 0;
  STCK(tempo_fine);  // save the end time
  pcb_attuale->p_time += tempo_fine - start_time[p_id];  // update time
  RELEASE_LOCK(&global_lock);
  *indirizzo_comando = value;
  klog_print(" DOIO pre scheduler ");
  scheduler();
  klog_print(" DOIO post scheduling ");
 
}

void GetCPUTime(state_t *stato, unsigned int p_id){    
  //Prendo dal campo p_time l' accumulated processor time usato dal processo che ha fatto questa syscall + il tempo già presente in p_time
  ACQUIRE_LOCK(&global_lock);  
  //Resetto i valori che ho memorizzato
  cpu_t reset_time = 0; 
  STCK(reset_time);
  current_process[p_id]->p_time += reset_time - start_time[p_id];
  current_process[getPRID()] = reset_time; //resetto il tempo di esecuzione del processo corrente
  //Devo metterlo nel registro a0 
  stato->reg_a0 = current_process[p_id]->p_time;   // metto il tempo di esecuzione del processo corrente nel registro a0
  stato->pc_epc += 4; // evito che vada in loop
  RELEASE_LOCK(&global_lock);
  LDST(stato);
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
    DoIO(stato, p_id);
    break;
   case GETTIME:
    GetCPUTime(stato, p_id);
    break;
   case CLOCKWAIT:
    break;
  case GETSUPPORTPTR:
    break;
  case GETPROCESSID:
    break;
  default: 
    PANIC();
    break;
   }
} else {
  klog_print("SyscallHandler > 0");
  stato->cause = GENERALEXCEPT; // perché non è possibile usare questi casi in usermode.
  programTrapHandler(GENERALEXCEPT, stato);
}
}


/*    
    }

    */



