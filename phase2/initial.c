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
void configureIRT(int line, int cpu){
    volatile memaddr *indirizzo_IRT = (volatile memaddr *)(IRT_START + (line * WS));
    *indirizzo_IRT |= IRT_RP_BIT_ON; // setta il bit di routing policy 1 << 28 in sostanza mette 1 a rp (|= non cancella la vecchia conf)
    *indirizzo_IRT |= (1 << cpu); // setta il bit di destinazione, 1 << cpu ...001 | ...010 | ...100
 }

void setTPR(int priority){
    volatile memaddr *tpr = (volatile memaddr *) TPR;
    *tpr = priority;
}

void configurePassupVector() {
    // ASSEGNIAMO FUORI DAL FOR IL VALORE DELLO STACK POINTER PER LA CPU 0
    passupvector_t *pvector = (passupvector_t *) PASSUPVECTOR;
    pvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    pvector->tlb_refill_stackPtr = KERNELSTACK;
    pvector->exception_handler = (memaddr)exceptionHandler;
    
    for (int cpu_id = 1; cpu_id < NCPU; cpu_id++) {
        pvector = (passupvector_t *)(PASSUPVECTOR + (cpu_id * 0x10));
        pvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
        pvector->tlb_refill_stackPtr = BASE_STACK0 + (cpu_id * PAGESIZE);
        pvector->exception_handler = (memaddr)exceptionHandler;
    }
}

//FUNZIONE DI INIZIALIZZAZIONE
void initializeSystem() {
    klog_print("Starting Nucleus.\n");
    initPcbs();
    initASL();
    mkEmptyProcQ(&pcbReady);
    for (int i = 0; i < NCPU; i++) {
        current_process[i] = NULL;
    }
    for (int i = 0; i < NRSEMAPHORES; i++) {
        sem[i] = (struct semd_t){0};
    }
    configurePassupVector();
}

// FUNZIONE CHE CREA IL PRIMO PROCESSO
void createFirstProcess() {
    pcb_t *first_pcb = allocPcb();

    first_pcb->p_s = (state_t){
        .pc_epc = (memaddr)test,
        .mie = MIE_ALL,
        .status = MSTATUS_MIE_MASK | MSTATUS_MPP_M,
    }; 
    RAMTOP(first_pcb->p_s.reg_sp);
    // Process tree fields, p_time, p_semAdd and p_supportStruct are already set to NULL/initialized by allocPcb()

    insertProcQ(&pcbReady, first_pcb);
    process_count++;
    /*
    pcb_t *first_process = allocPcb();
    state_t *stato = &first_process->p_s;
    stato->mie = MIE_ALL;
    stato->status = MSTATUS_MIE_MASK | MSTATUS_MPP_M;
  //  stato->reg_sp = RAMTOP(stato->reg_sp);
    RAMTOP(stato->reg_sp);
    stato->pc_epc = (memaddr)test;
    for (int i = 0; i < STATE_GPR_LEN; i++) {
        stato->gpr[i] = 0;
    }
    
    insertProcQ(&pcbReady, first_process);
    process_count++;
    */
}

void configureCPUs() {
    int cpucounter = 0;
    for (int i = 0; i < IRT_NUM_ENTRY; i++) {
        cpucounter = (i / (IRT_NUM_ENTRY / NCPU)) % NCPU;
        configureIRT(i, cpucounter);
    }
    
    setTPR(0);
    
    for (int i = 1; i < NCPU; i++) {
        current_process[i] = allocPcb();
        current_process[i]->p_s.status = MSTATUS_MPP_M;
        current_process[i]->p_s.pc_epc = (memaddr)scheduler;
        current_process[i]->p_s.reg_sp = BASE_STACK0 + (i * PAGESIZE);
        
        for (int j = 0; j < STATE_GPR_LEN; j++) {
            current_process[i]->p_s.gpr[j] = 0;
        }
        
        current_process[i]->p_s.entry_hi = 0;
        current_process[i]->p_s.cause = 0;
    }
}

int main(){
    klog_print("Avvio del Nucleus...\n");

    // Inizializzazione del sistema
    //punto 2.4
    initializeSystem();

    // Creazione del primo processo
    createFirstProcess();
    configureCPUs();
// Avvio dello scheduler
    // punto 2.9
    klog_print("Avvio dello scheduler...\n");
    scheduler();

    return 0;  // Non dovrebbe mai essere raggiunto
}