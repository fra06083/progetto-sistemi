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
extern void print(char *msg);
extern void p3test();
/*
//#include "../phase3/headers/initProc.h" // Phase 3 test
// #include "./p2test.c" // Phase 2 test
 //DEFINIZIONE DELLE VARIABILI GLOBALI
// FUNZIONI DI CONFIGURAZIONE
void configureIRT() {
    // Calcolo la maschera dei core (es. con 2 CPU -> 0b11)
    unsigned int cpu_mask = 0;
    for (int cpu = 0; cpu < NCPU; cpu++) {
        cpu_mask |= (1U << cpu);
    }

    // Inizializzo tutte le entry della IRT
    for (int i = 0; i < IRT_NUM_ENTRY; i++) {
        memaddr *irt_entry = (memaddr *)(IRT_START + i * WS);
        *irt_entry = 0;                // azzero l’entry
        *irt_entry |= IRT_RP_BIT_ON;   // bit "RP"
        *irt_entry |= cpu_mask;        // instrado a tutte le CPU
    }
}
void configurePassupVector() {
  passupvector_t *passupvector = (passupvector_t *) PASSUPVECTOR;
  for (int i = 0; i < NCPU; i++) {
    passupvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    if (i == 0) {
      passupvector->tlb_refill_stackPtr = KERNELSTACK;
      passupvector->exception_stackPtr = KERNELSTACK;
    } else {
      passupvector->tlb_refill_stackPtr = RAMSTART + (64 * PAGESIZE) + (i * PAGESIZE);
      passupvector->exception_stackPtr = 0x20020000 + (i * PAGESIZE);
    }
    passupvector->exception_handler = (memaddr)exceptionHandler;
    passupvector++; // teoricamente dovremmo fare passupvector += i*0x10 + PASSUPVECTOR? però non funziona in questo modo
  }
}


//FUNZIONE DI INIZIALIZZAZIONE
void initializeSystem() {
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
extern void test();
void createFirstProcess() {
  pcb_t *primo_processo = allocPcb();
  RAMTOP(primo_processo->p_s.reg_sp);  // Set SP to last RAM frame

  // Enable interrupts and kernel mode
  primo_processo->p_s.mie = MIE_ALL;
  primo_processo->p_s.status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M;
  primo_processo->p_s.pc_epc = (memaddr)p3test;
  insertProcQ(&pcbReady, primo_processo);
  process_count++;
}

void configureCPUs() {
  configureIRT();
  *((memaddr *)TPR) = 0;
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
  global_lock = 0; // inizializziamo il lock globale a 0
  // Creazione del primo processo
  createFirstProcess();
  configureCPUs();
  // Avvio dello scheduler
  // punto 2.9
  scheduler();

  return 0; // Non dovrebbe mai essere raggiunto
}
*/
// Our device_semaphores:
// DevNo:     01234567 | 01234567  | 01234567  | 01234567  | 01234567   | 01234567   | 0
// intLineNo: 0 - disk | 1 - flash | 2 - ether | 3 - print | 4 - term_o | 5 - term_i | pseudo-clock
int device_semaphores[NRSEMAPHORES];
const unsigned int PSEUDO_CLOCK_INDEX = NRSEMAPHORES-1;

volatile unsigned int global_lock;

extern void test(); // Phase 2 test entry point
extern void p3test(); // Phase 3 test entry point
extern void scheduler();
extern void exceptionHandler();

cpu_t current_process_start_time[NCPU]; // for each CPU it keeps the TOD of the last process that transitioned from ready to running on that CPU 

// End of declaration

int main()
{
    // 2. Pass Up Vectors population
    for (int i = 0; i < NCPU; i++) {
        passupvector_t v = {
            .tlb_refill_handler  = (memaddr)uTLB_RefillHandler,
            .tlb_refill_stackPtr = i == 0 ? KERNELSTACK : (RAMSTART + (64 * PAGESIZE) + (i * PAGESIZE)),
            .exception_handler   = (memaddr)exceptionHandler,
            .exception_stackPtr  = i == 0 ? KERNELSTACK : (0x20020000 + (i * PAGESIZE))
        };
        *((passupvector_t *)(PASSUPVECTOR + i*0x10)) = v;
    }

    // 3. Initialization
    initPcbs();
    initASL();

    // 4. Variables initialization
    process_count = 0;
    mkEmptyProcQ(&pcbReady);
    for (int i = 0; i < NCPU; i++) current_process[i] = NULL;
    for (int i = 0; i < NRSEMAPHORES; i++) device_semaphores[i] = 0;
    global_lock = 0;

    // 5. System-wide Interval Timer loading
    LDIT(PSECOND);

    // 6.  First PCB instantiation
    pcb_t *first_pcb = allocPcb();

    first_pcb->p_s = (state_t){
        .pc_epc = (memaddr)p3test, // Currently testing phase 3
        //.pc_epc = (memaddr)test, // Currently testing phase 2
        .mie = MIE_ALL,
        .status = MSTATUS_MPIE_MASK | MSTATUS_MPP_M,
    }; 
    RAMTOP(first_pcb->p_s.reg_sp);
    // Process tree fields, p_time, p_semAdd and p_supportStruct are already set to NULL/initialized by allocPcb()

    insertProcQ(&pcbReady, first_pcb);
    process_count++;

    // 7. Interrupt routing - each IRT entry has the RP bit and the first 8 least significative bits set to 1
    unsigned int bits = 0;
    for (int cpu = 0; cpu < NCPU; cpu++)
        bits += (1 << cpu);
    for (int i = 0; i < IRT_NUM_ENTRY; i++) {
        memaddr entry = IRT_START + i*WS; 
        *((memaddr *)entry) = 0; // just to be sure that the entry is 0 before initializing it
        *((memaddr *)entry) |= IRT_RP_BIT_ON;
        *((memaddr *)entry) |= bits;
    }

    *((memaddr *)TPR) = 0;

    // 8. CPUs state setting
    state_t start_state = {
        .status = MSTATUS_MPP_M,
        .pc_epc = (memaddr)scheduler,
        .gpr = {0},
        .entry_hi = 0,
        .cause = 0,
        .mie = 0
    };

    for (int i = 1; i < NCPU; i++) {
        start_state.reg_sp = (0x20020000 + i * PAGESIZE);
        INITCPU(i, &start_state);
    }

    // 9. Scheduler calling
    scheduler();

    return 0;
}