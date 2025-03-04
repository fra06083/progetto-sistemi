#include "./headers/pcb.h"
#include "./headers/asl.h"
#include "./headers/types.h"
#include "./headers/listx.h"
#include "./headers/const.h"
#include "header/exceptions.h"
#include <uriscv/liburiscv.h> // libreria di uriscv, viene richiesta, sennò non abbiamo le funzioni che ci servono LDST
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
#define BASE_STACK0 0x2000200 // Inizio dello stack
#define NCPU 8
extern struct list_head pcbReady; // Lista dei processi pronti
extern int processCount = 0; // Contatore dei processi
extern pcb_t* currentProcess[8]; // Vettore di puntatori, 8 processi che vanno nelle varie CPU
extern int globalLock = 0; // Lock globale
extern semd_PTR terminal; // Semd per il terminale ne richiede 2 per terminale
extern semd_PTR pseudoClock; // Semd per il clock
extern void test();
int main (){
// inizializziamo i processi e i semafori
    initPcbs();
    initASL();
    
    passupvector_t* passupvector; // abbiamo preso il passupvector da types.h di uriscv (dichiarato nell'include di headers/types.h)
    passupvector->tlb_refill_handler = (memaddr) uTLB_RefillHandler();
    passupvector->tlb_refill_stackPtr = BASE_STACK0;
    int stack_ptr_cpu[8]; // stack pointer per ogni cpu
    stack_ptr_cpu[0] = KERNELSTACK;
    for (int cpu_id = 1; cpu_id <= NCPU;cpu_id++){
        //0x20020000 + (cpu_id * PAGESIZE)
        stack_ptr_cpu[cpu_id]= stack_ptr_cpu[cpu_id] + (cpu_id * PAGESIZE);
        passupvector->tlb_refill_stackPtr = stack_ptr_cpu[cpu_id]; 
    }
    
    // qua non capiamo cosa fare con il passupvector, quale degli 8 ci dobbiamo riferire? dopo?
    // il puntatore si trova facendo BASE_STACK0 + cpu_id * PAGESIZE
    passupvector->exception_handler = (memaddr) exceptionHandler();
    passupvector->exception_stackPtr = passupvector->tlb_refill_stackPtr + PAGESIZE;
    //pcbReady = removeBlocked(pcbFree_table);
    /* 
    Sta parte serve per fare lo stack pointer per la parte del nucleo TLB-Refill Handler

    in sostanza faremo uso di un for per accedere alle varie pagine dedicate ad ogni cpu.
    for (int i = (partiamo dalla prima cpu che è 1); i<=NCPU;i++);
    */

     /* iniziamo il timer di psuedoclock, in sostanza dopo che LDIT fa 0, dobbiamo sbloccare i processi LDIT(PSECOND); // carichiamo il timer
     quando arriva a 0 dobbiamo sbloccare i processi, in sostanza dobbiamo fare una V sul semaforo pseudoClock per poi fare il passaggio di controllo con LDST(processo);
     */
    return 0;
}