/*
SUDDIVISIONE DEI FILE
13.1 Module Decomposition
One possible module decomposition is as follows:
1. initial.c This module implements main() and exports the Nucleus’s global variables (e.g.
process count, Global Lock, blocked PCBs lists/pointers, etc.).
2. scheduler.c This module implements the Scheduler.
3. interrupts.c This module implements the device/timer interrupt exception handler.
4. exceptions.c This module implements the TLB, Program Trap, and SYSCALL exception
handlers. Furthermore, this module will contain the provided skeleton TLB-Refill event handler
(e.g. uTLB_RefillHandler)

*/
#include "./headers/initial.h"
 //DEFINIZIONE DELLE VARIABILI GLOBALI
// FUNZIONI DI CONFIGURAZIONE
void configureIRT(int line, int cpu) {
  for (int line = 0; line < N_INTERRUPT_LINES; line++) {  // dobbiamo farlo per ogni linea di interruzione
    for (int dev = 0; dev < N_DEV_PER_IL; dev++) {        // per ogni device
      memaddr *irt_entry = (memaddr *)IRT_ENTRY(line, dev);
      *irt_entry = IRT_RP_BIT_ON;                                                
      for (int cpu_id = 0; cpu_id < NCPU; cpu_id++){
        *irt_entry |= (1U << cpu_id); // setta il bit di destinazione, 1 << cpu ...001 | ...010 | ...100; in fase finale era sbagliata prima
      }  
    }
  }

  /*
  volatile memaddr * indirizzo_IRT = (volatile memaddr * )(IRT_START + (line * WS));
  * indirizzo_IRT |= IRT_RP_BIT_ON; // setta il bit di routing policy 1 << 28 in sostanza mette 1 a rp (|= non cancella la vecchia conf)
  * indirizzo_IRT |= (1 << cpu); // setta il bit di destinazione, 1 << cpu ...001 | ...010 | ...100
*/
}
void configurePassupVector() {
passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;
for (int i = 0; i < NCPU; i++) {
  passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
  if (i == 0) {
    passupvector->tlb_refill_stackPtr = KERNELSTACK;
  } else {
    passupvector->tlb_refill_stackPtr = RAMSTART + (64 * PAGESIZE) + (i * PAGESIZE);
  }
  passupvector->exception_handler = (memaddr)exceptionHandler;
  if (i == 0) {
    passupvector->exception_stackPtr = KERNELSTACK;
  } else {
    passupvector->exception_stackPtr = 0x20020000 + (i * PAGESIZE);
  }
  passupvector++; // qui avevamo sbagliato, dovevamo semplicemente incrementare il puntatore
}

}


//FUNZIONE DI INIZIALIZZAZIONE
void initializeSystem() {
  klog_print("Starting Nucleus.\n");
  configurePassupVector(); // PUNTO DUE
  initPcbs();
  initASL();
  mkEmptyProcQ(&pcbReady);
  for (int i = 0; i < NCPU; i++) {
    current_process[i] = NULL;
    start_time[i] = 0; // Inizializziamo il tempo di inizio di ogni processo
  }
  for (int i = 0; i < NRSEMAPHORES; i++) {
    sem[i] = 0;
  }
  LDIT(PSECOND);
}

// FUNZIONE CHE CREA IL PRIMO PROCESSO
void createFirstProcess() {
  pcb_t *primo_processo = allocPcb();
  RAMTOP(primo_processo->p_s.reg_sp);  // Set SP to last RAM frame

  // Enable interrupts and kernel mode
  primo_processo->p_s.mie = MIE_ALL;
  primo_processo->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
  primo_processo->p_s.pc_epc = (memaddr)test;
  insertProcQ(&pcbReady, primo_processo);
  process_count++;
}

void configureCPUs() {
  for (int i = 0; i < IRT_NUM_ENTRY; i++) {
    int cpucounter = (i / (IRT_NUM_ENTRY / NCPU)) % NCPU;
    configureIRT(i, cpucounter);
  }
 // *((memaddr *)TPR) = 0;
  state_t stato;
  stato.status = MSTATUS_MPP_M;
  stato.pc_epc = (memaddr) scheduler;
  stato.entry_hi = 0;
  stato.cause = 0;
  stato.mie = 0;

   for (int i = 1; i < NCPU; i++) {
    stato.reg_sp = (0x20020000 + (i * PAGESIZE));
    INITCPU(i, &stato);
  }
}

int main() {

  // Inizializzazione del sistema
  //punto 2.4
  initializeSystem();
  global_lock = 0; // inizializziamo il lock globale a 1
  // Creazione del primo processo
  createFirstProcess();
  configureCPUs();
  // Avvio dello scheduler
  // punto 2.9
  scheduler();

  return 0; // Non dovrebbe mai essere raggiunto
}