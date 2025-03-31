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

// FILE FASE 1
#include "../phase1/headers/pcb.h"
#include "../phase1/headers/asl.h"
#include "../headers/const.h"
#include "../klog.c"


// FILE URISCV
#include "uriscv/arch.h"
#include "uriscv/cpu.h"
#include <uriscv/liburiscv.h> // libreria di uriscv, viene richiesta, sennò non abbiamo le funzioni che ci servono LDST

// FILE SECONDA FASE
#include "./exceptions.c"
#include "./interrupts.c"
#include "./scheduler.c"

#include "headers/exceptions.h" // da vedere se serve

#define BASE_STACK0 0x2000200 // Inizio dello stack
int process_count = 0;        // Contatore dei processi
struct pcb_t *current_process[NCPU];  // Vettore di puntatori, 8 processi che vanno nelle varie CPU
struct semd_t sem[NRSEMAPHORES];
volatile unsigned int global_lock; // Lock globale
struct list_head pcbReady;         // Lista dei processi pronti
// extern perché sennò darebbe errore il compilatore
// extern fa capire solamente che la funzione è definita in un altro file

extern void test();
extern void scheduler();
extern void exceptionHandler();
extern void interruptHandler();
extern void uTLB_RefillHandler();
// ASSEGNIAMO FUORI DAL FOR IL VALORE DELLO STACK POINTER PER LA CPU 0
passupvector_t *pvector = (passupvector_t *) PASSUPVECTOR;

// PUNTO 7 rivedere
// da riguardare
void configureIRT(int line, int cpu){
    volatile memaddr *irt_entry = (volatile memaddr *) IRT_ENTRY(line, cpu);
    memaddr entry = (1 << IRT_ENTRY_POLICY_BIT) | (line << IRT_ENTRY_DEST_BIT);
    *irt_entry = entry;
    //
}

void setTPR(int priority){
    volatile memaddr *tpr = (volatile memaddr *) TPR;
    *tpr = priority;
}

int main(){
    klog_print("Starting Nucleus.\n");
    //   int stack_ptr_cpu[8]; // stack pointer per ogni cpu
    //  stack_ptr_cpu[0] = KERNELSTACK; // stack pointer non serve secondo me, il tutor ha scritto che il passupvector
    // deve esserci per ogni cpu, quindi non ha senso fare un vettore di stack pointer int
    pvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    pvector->tlb_refill_stackPtr = KERNELSTACK;
    pvector->exception_handler = (memaddr)exceptionHandler; // handler delle eccezioni, dobbiamo farla noi
    passupvector_t *pvector = (passupvector_t *)(PASSUPVECTOR + 0x10);
    klog_print("Inizializzazione pvector...\n");
    for (int cpu_id = 1; cpu_id < NCPU; cpu_id++)
    {
        // 0x20020000 + (cpu_id * PAGESIZE)
        pvector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
        pvector->tlb_refill_stackPtr = BASE_STACK0 + (cpu_id * PAGESIZE);
        pvector->exception_handler = (memaddr)exceptionHandler; // handler delle eccezioni, dobbiamo farla noi
        // 0x10 è la dimensione di passupvector, quindi ci spostiamo di 16 byte
        passupvector_t *pvector = (passupvector_t *)(PASSUPVECTOR + (cpu_id * 0x10));
    }
    // inizializziamo le strutture dati
    initPcbs();
    initASL();
    klog_print(" init delle strutture dati..\n");
    
    // PUNTO 4 Inizializziamo ora le variabili globali
    mkEmptyProcQ(&pcbReady);
    for (int i = 0; i < NRSEMAPHORES; i++){
        
        sem[i] = (struct semd_t){0};
    }
    for (int i = 0; i < NCPU; i++){

        current_process[i] = NULL;
    }
    global_lock = 1; // Inizializziamo a 1 perché sennò non andrebbe, è un lock. (semaforo)
    // PUNTO 5 Inizializziamo ora l'interval timer
    LDIT(PSECOND); // psecond è un valore costante è 100 ms.
    // Inizializziamo ora il primo processo
    pcb_t *first_process = allocPcb();
    state_t *stato = &first_process->p_s;
    // Abilito le interruzioni come dice il file delle specifiche
    stato->mie = MIE_ALL;

    // Imposta la modalità kernel e abilita le interruzioni
    stato->status = MSTATUS_MIE_MASK | MSTATUS_MPP_M;
    stato->reg_sp = RAMTOP(stato->reg_sp);
    // Imposta il Program Counter all'indirizzo di test di pcb2test
    stato->pc_epc = (memaddr)test;
    // A questo punto, inizializziamo i registri generali, se necessario.
    // Esempio (potresti inizializzare i GPR con 0):
    for (int i = 0; i < STATE_GPR_LEN; i++){
        stato->gpr[i] = 0;
    }
    // settiamo lo stato e inseriamo il processo nella ready queue; è il primo processo possibile, lo creiamo all'inizio
    // PAGINA 3 DEL PDF punto 3
    insertProcQ(&pcbReady, first_process);
    process_count++;
    current_process[0] = first_process;
    // Punto 7 interrupt routing table sistemiamo
    // Abbiamo l'indirizzo di IRT
    /*
    Use *((memaddr)TPR) = 0 and *((memaddr)TPR) = 1
    to set the Task Priority Register.
    Tabella che mappa
    classe dispositivi per ogni linea di interrupt

    */
    // WS è la word size (che è 4 byte)
    // 6 REGISTRI PER CPU
    int cpucounter =  -1;


    for (int i = 0; i < IRT_NUM_ENTRY; i++) {
        klog_print("s \n");
        if (i % (IRT_NUM_ENTRY / NCPU) == 0) {
            cpucounter++;  // Cambia CPU dopo ogni gruppo di entry
        }

        // Configura la linea di interrupt i per la CPU cpucounter
      configureIRT(i, cpucounter);
    }
    klog_print("PRIMA DEL TPR \n");
    setTPR(0);
    // il tpr viene impostato a 0 perché è ready
    klog_print(" TPR\n");
    // PUNTO 8
    for (int i = 1; i < NCPU; i++){
        current_process[i] = allocPcb();
        current_process[i]->p_s.status = MSTATUS_MPP_M;
        current_process[i]->p_s.pc_epc = (memaddr)scheduler;
        current_process[i]->p_s.reg_sp = BASE_STACK0 + (i * PAGESIZE);
        for (int i = 0; i < STATE_GPR_LEN; i++)
        {
            current_process[i]->p_s.gpr[i] = 0;
        }
        current_process[i]->p_s.entry_hi = 0;
        current_process[i]->p_s.cause = 0;
    }
    // NRSEMAPHORES % 2
    // PARTE FINALE dell'initial: ora possiamo iniziare a fare il ciclo di scheduling
    klog_print("PARTE LO SCHEDULER");
    scheduler();
    // qui è finito, non deve ritornare nel main sennò è errore

    // MANCA UNA PARTE DEL CODICE non è finito
    return 0;
}