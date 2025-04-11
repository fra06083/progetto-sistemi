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
// int lock_0;
// FUNZIONI DI CONFIGURAZIONE
extern void test();
void configureIRT(int line, int cpu) {
  volatile memaddr * indirizzo_IRT = (volatile memaddr * )(IRT_START + (line * WS));
  * indirizzo_IRT |= IRT_RP_BIT_ON; // setta il bit di routing policy 1 << 28 in sostanza mette 1 a rp (|= non cancella la vecchia conf)
  * indirizzo_IRT |= (1 << cpu); // setta il bit di destinazione, 1 << cpu ...001 | ...010 | ...100
}

void configurePassupVector() {
 for (int i = 0; i < NCPU; i++) {
  passupvector_t v;
  v.exception_handler = (memaddr) exceptionHandler;
  v.tlb_refill_handler = (memaddr) uTLB_RefillHandler;
  if (i == 0) {
    v.tlb_refill_stackPtr = KERNELSTACK;
    v.exception_stackPtr  = KERNELSTACK;
 } else { 
    v.tlb_refill_stackPtr = RAMSTART + (64 * PAGESIZE) + (i * PAGESIZE);
    v.exception_stackPtr  = BASE_STACK0 + (i * PAGESIZE);
  }
  // Scrive direttamente nella memoria mappata
 *((passupvector_t *)(PASSUPVECTOR + i * 0x10)) = v;
}
}


//FUNZIONE DI INIZIALIZZAZIONE
void initializeSystem() {
  klog_print("Starting Nucleus.\n");
  initPcbs();
  initASL();
  mkEmptyProcQ( & pcbReady);
  for (int i = 0; i < NCPU; i++) {
    current_process[i] = NULL;
  }
  for (int i = 0; i < NRSEMAPHORES; i++) {
    sem[i] = (struct semd_t) {
      0
    };
  }
  LDIT(PSECOND);
  configurePassupVector();
}

// FUNZIONE CHE CREA IL PRIMO PROCESSO
void createFirstProcess() {
  pcb_t * first_process = allocPcb();
  (first_process -> p_s).mie = MIE_ALL;
  (first_process -> p_s).status = MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  //  stato->reg_sp = RAMTOP(stato->reg_sp);
  RAMTOP((first_process -> p_s).reg_sp);
  (first_process -> p_s).pc_epc = (memaddr) test;
  insertProcQ( & pcbReady, first_process);
  process_count++;
}

void configureCPUs() {
  for (int i = 0; i < IRT_NUM_ENTRY; i++) {
    int cpucounter = (i / (IRT_NUM_ENTRY / NCPU)) % NCPU;
    configureIRT(i, cpucounter);
  }
  *((memaddr *)TPR) = 0;
  state_t stato;
  stato.status = MSTATUS_MPP_M;
  stato.pc_epc = (memaddr) scheduler;
  for (int j = 0; j < STATE_GPR_LEN; j++) {
    stato.gpr[j] = 0;
  }
  stato.entry_hi = 0;
  stato.cause = 0;
  stato.mie = 0;
  ACQUIRE_LOCK( & global_lock);
  lock_cpu0 = 1; // prenndo il lock così sbloccando le cpu nello scheduler si bloccano al primo if. Sennò prenderebbero il programma test
  for (int i = 1; i < NCPU; i++) {
    stato.reg_sp = (0x20020000 + i * PAGESIZE);
    INITCPU(i, & stato);
  }
  klog_print("CPU settate!\n");
}

int main() {
  klog_print("Avvio del Nucleo:\n");

  // Inizializzazione del sistema
  //punto 2.4
  initializeSystem();

  // Creazione del primo processo
  createFirstProcess();
  configureCPUs();
  // Avvio dello scheduler
  // punto 2.9
  klog_print("Avvio dello scheduler:\n");
  scheduler();

  return 0; // Non dovrebbe mai essere raggiunto
}