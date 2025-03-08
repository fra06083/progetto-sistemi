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
// File di uriscv
#include <uriscv/cpu.h>
#include <uriscv/arch.h>
#include <uriscv/liburiscv.h> // libreria di uriscv, viene richiesta, sennò non abbiamo le funzioni che ci servono LDST


// implemento i file di fase 1
#include "../klog.c"
#include "../phase1/headers/pcb.h"
#include "../phase1/headers/asl.h"
#include "../headers/const.h"


// File fase 2
#include "headers/exceptions.h"
#include "headers/interrupts.h"
#include "headers/scheduler.h"
#include "./p2test.c"

#define BASE_STACK0 0x2000200 // Inizio dello stack
int process_count = 0; // Contatore dei processi
struct list_head ready_queue;
struct pcb_t *current_process[NCPU];
struct semd_t sem[NRSEMAPHORES];
volatile unsigned int global_lock; // Lock globale
struct list_head pcbReady; // Lista dei processi pronti
int processCount = 0; // Contatore dei processi
pcb_t* currentProcess[8]; // Vettore di puntatori, 8 processi che vanno nelle varie CPU
semd_PTR terminal; // Semd per il terminale ne richiede 2 per terminale
semd_PTR pseudoClock; // Semd per il clock

// extern perché sennò darebbe errore il compilatore
// extern fa capire solamente che la funzione è definita in un altro file
extern void test();
extern void scheduler();
extern void uTLB_RefillHandler();
extern void exceptionHandler();
// ASSEGNIAMO FUORI DAL FOR IL VALORE DELLO STACK POINTER PER LA CPU 0
passupvector_t *pvector = (passupvector_t *) PASSUPVECTOR;
int main (){
//   int stack_ptr_cpu[8]; // stack pointer per ogni cpu
//  stack_ptr_cpu[0] = KERNELSTACK; // stack pointer non serve secondo me, il tutor ha scritto che il passupvector
// deve esserci per ogni cpu, quindi non ha senso fare un vettore di stack pointer int
    pvector->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
    pvector->tlb_refill_stackPtr = KERNELSTACK;
    pvector->exception_handler = (memaddr) exceptionHandler; // handler delle eccezioni, dobbiamo farla noi
    for (int cpu_id = 1; cpu_id < NCPU;cpu_id++){
        //0x20020000 + (cpu_id * PAGESIZE)
        pvector->tlb_refill_handler = (memaddr) uTLB_RefillHandler;
        pvector->tlb_refill_stackPtr = BASE_STACK0 + (cpu_id * PAGESIZE);
        pvector->exception_handler = (memaddr) exceptionHandler; // handler delle eccezioni, dobbiamo farla noi
        // 0x10 è la dimensione di passupvector, quindi ci spostiamo di 16 byte
        passupvector_t *pvector = (passupvector_t *)(PASSUPVECTOR + cpu_id * 0x10);
    }
    // inizializziamo le strutture dati
    initPcbs();
    initASL();

    // Inizializziamo ora le variabili globali
    mkEmptyProcQ(&pcbReady);
    for (int i = 0; i < NRSEMAPHORES; i++){
        sem[i] = (struct semd_t){0};
    }
    for (int i = 0; i < NCPU; i++){ 
        current_process[i] = NULL;
    }
    global_lock = 0;
    // Inizializziamo ora l'interval timer
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
    stato->pc_epc = (unsigned int)test;

    // A questo punto, puoi inizializzare altri campi del PCB, come i registri generali, se necessario.
    // Esempio (potresti inizializzare i GPR con 0):
    for (int i = 0; i < STATE_GPR_LEN; i++) {
        stato->gpr[i] = 0;
    }
    // settiamo lo stato e inseriamo il processo nella ready queue; è il primo processo possibile, lo creiamo all'inizio
    // PAGINA 3 DEL PDF punto 3
    insertProcQ(&ready_queue, first_process);
    process_count++;
    // QUI MANCA ROBA CONTINUA DALLA PAGINA 3 DAL PUNTO 7




    // PARTE FINALE dell'initial: ora possiamo iniziare a fare il ciclo di scheduling
    scheduler();
    // qui è finito, non deve ritornare nel main sennò è errore

    // MANCA UNA PARTE DEL CODICE non è finito
    return 0;
}