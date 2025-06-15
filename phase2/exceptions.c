#include "./headers/exceptions.h"

/*

Important: To determine if the Current Process was executing in kernel-mode or user-mode,
one examines the Status register in the saved exception state. In particular, examine the MPP field
if the status register with saved_exception_state->status & MSTATUS_MPP_MASK see section Dott.
Rovelli’s thesis for more details.
*/

// il vpn è nei 31..12 bit di entry_hi, lo utilizzaimo come index della page table contenuta 
// in p_supportStruct
void uTLB_ExceptionHandler(){
  int cpu_id= getPRID();
  support_t *sup = (support_t *)current_process[cpu_id]->p_supportStruct;

  unsigned int entry_hi = sup->sup_exceptState[0].entry_hi; //utilizzaimo sup_exceptState[0] perchè le eccezioni sono tbl 0 o general 1 exception
  unsigned int vpn = entry_hi  >> 12; //estrae il VPN dall'entry_hi

  int index;
  if(vpn==0x8FFFF) index = 31; // Stack area
  else index = vpn - 0x80000; // text e data area

  if(index<0 || index>=USERPGTBLSIZE){
    // Indice fuori dai limiti della page table
    SYSCALL(TERMPROCESS,0, 0, 0); // Chiamata di sistema per gestire l'errore
  }

  pteEntry_t entry= sup->sup_privatePgTbl[index]; // Ottiene l'entry della page table

  setENTRYHI(entry.pte_entryHI); // Imposta l'entry HI
  setENTRYLO(entry.pte_entryLO); // Imposta l'entry LO
  TLBWR(); // Scrive l'entry nella TLB

  //restituisce il controllo al processo corrente
  LDST(&sup->sup_exceptState[0]); 

}

void passupordie(int cause, state_t* stato){
  ACQUIRE_LOCK(&global_lock);
  int cpu_id = getPRID();
  pcb_t* current = current_process[cpu_id];

  if (current == NULL) {
    RELEASE_LOCK(&global_lock);
    scheduler();
  }

  if (current->p_supportStruct == NULL) {
    killProcess(current);
    RELEASE_LOCK(&global_lock);
    scheduler();
  }

  // Copia dell'eccezione
  current->p_supportStruct->sup_exceptState[cause] = *stato;
  context_t* context = &current->p_supportStruct->sup_exceptContext[cause];
  RELEASE_LOCK(&global_lock);
  LDCXT(context->stackPtr, context->status, context->pc); // qui carica il context
  // quindi mettiamo il release lock prima
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
    state_t *stato = GET_EXCEPTION_STATE_PTR(getPRID()); // prendiamo lo state dell'eccezione
    int getExcepCode = getCAUSE() & CAUSE_EXCCODE_MASK;
    if (CAUSE_IS_INT(getCAUSE())) { // controlliamo se è un interrupt
      interruptHandler(getExcepCode, stato); 
    } else {        
      if (getExcepCode >= 24 && getExcepCode <= 28) {
          passupordie(PGFAULTEXCEPT, stato); // Page fault exception
      } else if (getExcepCode == 8 || getExcepCode == 11) {
          syscallHandler(stato); // syscall handler
      } else if ((getExcepCode >= 0 && getExcepCode <= 7) || 
                  getExcepCode == 9 || getExcepCode == 10 || 
                 (getExcepCode >= 12 && getExcepCode <= 23)) {
          passupordie(GENERALEXCEPT, stato); // general exception
      }
    }
}

void createProcess(state_t *c_state){
  ACQUIRE_LOCK(&global_lock);
  pcb_t *new_process = allocPcb();
  if (new_process == NULL) {
    c_state->reg_a0 = -1; // restituisco -1 nel registro a0 se non posso creare un processo
    c_state->pc_epc += 4;
    LDST(c_state);
    RELEASE_LOCK(&global_lock);
    return; // non possiamo creare un processo, quindi esce fuori dalla funzione
  }
  state_t *p_s = (state_t *)c_state->reg_a1;
  // sbagliato new_process->p_s = c_state->reg_a1;
  new_process->p_s = *p_s;
  new_process->p_supportStruct = (support_t *) c_state->reg_a3;
  if (current_process[getPRID()] != NULL) {
    insertChild(current_process[getPRID()], new_process);
  }
  insertProcQ(&pcbReady, new_process); // va aggiunto alla ready queue!!
  process_count++;
  c_state->reg_a0 = new_process->p_pid;
  c_state->pc_epc += 4; // lo dice nel punto dopo
  RELEASE_LOCK(&global_lock);
  LDST(c_state);
}
void terminateProcess(state_t *c_state, unsigned int p_id){
  // dobbiamo terminare il processo corrente
  ACQUIRE_LOCK(&global_lock);
  if (p_id==0){
    // terminiamo il processo
    killProcess(current_process[p_id]);
    RELEASE_LOCK(&global_lock);
    scheduler(); // va nello scheduler se abbiamo terminato il processo corrente
  } else { 
    // CERCHIAMO il processo, mettiamo true così lo rimuove di già
  pcb_t *process = findProcess(p_id);
  if (process != NULL){
    killProcess(process); // non lo abbiamo trovato, il termprocess vuole terminare un processo che non esiste
    // dovremo far ripartire l'esecuzione del processo ma come??
    RELEASE_LOCK(&global_lock);
    scheduler();
    return;
  }
  RELEASE_LOCK(&global_lock);
  // dovremo far ripartire l'esecuzione del processo ma come??
  c_state->pc_epc += 4;             // increment the program counter
  LDST(c_state);
  }
}

void P(state_t* stato, unsigned int p_id) {

  int *semaforo = (int *) stato->reg_a1;
  (*semaforo)--;  // decrementa il semaforo

  if (*semaforo < 0) {
      // Blocca il processo corrente
      pcb_t* pcbBlocked = current_process[p_id];
      ACQUIRE_LOCK(&global_lock);
      insertBlocked(semaforo, pcbBlocked);  // inserisci il processo nella lista dei bloccati
      block(stato, p_id, pcbBlocked);  // blocca il processo corrente
      RELEASE_LOCK(&global_lock);
      scheduler(); 
      return;  // ritorna e non continuare l'esecuzione del processo corrente
  } else if (*semaforo >= 1) {
      ACQUIRE_LOCK(&global_lock);
      pcb_t* processo_sbloccato = removeBlocked(semaforo);  // sblocca il processo in attesa
      if (processo_sbloccato != NULL) {
        insertProcQ(&pcbReady, processo_sbloccato);  // metti il processo nella coda dei ready
      }
      RELEASE_LOCK(&global_lock);
  }
  stato->pc_epc += 4;  // incrementa il program counter
  LDST(stato);  // ripristina lo stato del processo
}

void V(state_t* stato, unsigned int p_id) {

  int *semaforo = (int *) stato->reg_a1;
  (*semaforo)++;  // incrementa il semaforo
  if (*semaforo > 1){
  // Se il semaforo è maggiore di 1, blocca il processo chiamante (V bloccante)
    ACQUIRE_LOCK(&global_lock);
    insertBlocked(semaforo, current_process[p_id]);
    block(stato, p_id, current_process[p_id]);  // blocca il processo corrente
    RELEASE_LOCK(&global_lock);
    scheduler();
    return;  // ritorna e non continuare l'esecuzione del processo corrente

  } else {
    // Sblocca il processo in attesa
    ACQUIRE_LOCK(&global_lock);
    pcb_t* processo_sbloccato = removeBlocked(semaforo);
    if (processo_sbloccato != NULL) {
        insertProcQ(&pcbReady, processo_sbloccato);  // metti il processo nella coda dei ready
    }
    RELEASE_LOCK(&global_lock);
  }
  stato->pc_epc += 4;  // incrementa il program counter
  LDST(stato);  // ripristina lo stato del processo
}
void DoIO(state_t *stato, unsigned int p_id){
  // Questo servizio richiede al Nucleo di eseguire un'operazione di I/O su un dispositivo.
  // Dobbiamo implementare il codice per gestire questa operazione
  ACQUIRE_LOCK(&global_lock);
  memaddr* indirizzo_comando = (memaddr*) stato->reg_a1;  // prendiamo il comando e il suo valore
  int v = stato->reg_a2;                

  if (indirizzo_comando == NULL) {
    stato->reg_a0 = -1;  // ritorna -1 se nullo
    stato->pc_epc += 4;  // incrementa il program counter
    RELEASE_LOCK(&global_lock);
    LDST(stato);
    return;
  }

  pcb_t* pcb_attuale = current_process[p_id];
  int devIndex = findDevice(indirizzo_comando - 1);  // troviamo il dispositivo
  if (devIndex < 0) {
    stato->reg_a0 = -1;  // dispositivo non valido, -1 in reg a0
    stato->pc_epc += 4;  // antiloop
    RELEASE_LOCK(&global_lock);
    LDST(stato);
    return;
  }
  /* P semplificata in i/o */
  int* semPTR = &sem[devIndex];  // prendi il semaforo del dispositivo
  (*semPTR)--;   // decrementa il semaforo per bloccare il processo fino a quando l'operazione input/output è completata
  stato->pc_epc += 4; 
  pcb_attuale->p_s = *stato;
  insertBlocked(semPTR, pcb_attuale);  // inseisci il processo nei bloccati
  current_process[p_id] = NULL;         //  rimuovi il processo corrente dalla lista dei processi attivi
  pcb_attuale->p_time = getTime(p_id);  // update tempo di esecuzione
  *indirizzo_comando = v;  // scrivi il valore nel dispositivo
  RELEASE_LOCK(&global_lock);
  scheduler();
  return;  // ritorna alla funzione chiamante
}
void GetCPUTime(state_t *stato, unsigned int p_id){    
  //Prendo dal campo p_time l' accumulated processor time usato dal processo che ha fatto questa syscall + il tempo già presente in p_time
  ACQUIRE_LOCK(&global_lock);  
  cpu_t time = getTime(p_id);
  stato->reg_a0 = current_process[p_id]->p_time + time;
  stato->pc_epc += 4; // evito che vada in loop
  RELEASE_LOCK(&global_lock);
  LDST(stato);
}

void WaitForClock(state_t *stato, unsigned int p_id){ 
  ACQUIRE_LOCK(&global_lock);
  int *clock = &sem[PSEUDOSEM]; // indirizzo del semaforo dello PseudoClock
  insertBlocked(clock, current_process[p_id]); // inserisci il processo nella lista dei bloccati (ASL), verrano sbloccati dall'interrupt
  block(stato, p_id, current_process[p_id]); // blocca il processo corrente
  RELEASE_LOCK(&global_lock);
  scheduler(); // Chiamata allo scheduler per scegliere il prossimo processo

}

void GetSupportData(state_t *stato, unsigned int p_id){
  //Restituisco in a0 il puntatore (se diverso da NULL) alla support struct (è un indirizzo di memoria) del PCB corrent sulla CPU 
  ACQUIRE_LOCK(&global_lock);
  support_t* pcbsuppStruct = current_process[p_id]->p_supportStruct;
  //Inizializzata a NULL in allocPCB() controllo sul suo valore inutile, verrà passato NULL o un altro valore senza problemi 
  stato->reg_a0 = (memaddr) pcbsuppStruct; // ci vuole il cast a memaddr
  RELEASE_LOCK(&global_lock);
  stato->pc_epc += 4; 
  LDST(stato); 
}

void GetProcessId(state_t *stato, unsigned int p_id){       //Rivedere ok
  //Se il parent del pcb che ha fatto la syscall è NULL, allora in reg_a0 ci deve essere il suo PID
  ACQUIRE_LOCK(&global_lock);
  int parent = stato->reg_a1;
  pcb_t *currpcb = current_process[p_id];
  if (parent) { // Dentro l'if allora il parent non era 0
      pcb_t* pcb_parent = currpcb->p_parent;
      if (pcb_parent == NULL){
        stato->reg_a0 = 0;
      } else {
        stato->reg_a0 = pcb_parent->p_pid;
      }
  } else { // parent è 0, quindi restituisco il pid del processo corrente
      stato->reg_a0 = currpcb->p_pid;
  }
  stato->pc_epc += 4;
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
int p_id = getPRID();
int registro = stato->reg_a0;
if (registro > 0){
  passupordie(GENERALEXCEPT, stato);
}
if (!(stato->status & MSTATUS_MPP_MASK)) { // E' in user mode (fase 3??)
  // allora dobbiamo fare un'azione descritta nella pagina 7 del pdf
  stato->cause = PRIVINSTR;
  exceptionHandler();
}
switch (registro){
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
    WaitForClock(stato, p_id);
    break;
   case GETSUPPORTPTR:
    GetSupportData(stato, p_id); 
    break;
   case GETPROCESSID:
    GetProcessId(stato, p_id); 
    break;
   default: 
    PANIC(); // Se non è nessuna delle syscall allora PANIC, non dovrebbe arrivare mai qui
    break;
   }
}


