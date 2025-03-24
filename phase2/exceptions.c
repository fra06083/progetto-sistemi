#include "./headers/exceptions.h"
#include "/usr/include/uriscv/types.h"
/*

Important: To determine if the Current Process was executing in kernel-mode or user-mode,
one examines the Status register in the saved exception state. In particular, examine the MPP field
if the status register with saved_exception_state->status & MSTATUS_MPP_MASK see section Dott.
Rovelli’s thesis for more details.

*/
extern void interruptHandler();


void uTLB_ExceptionHandler(){


}
void programTrapHandler(){

}
void exceptionHandler()
{
    
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
        syscallHandler(getExcepCode);
    } else {
        /* Nucleus’s Program Trap exception handler */
        programTrapHandler();
    }
}
void createProcess(state_t *c_state){
  ACQUIRE_LOCK(&global_lock);
  pcb_t *new_process = allocPcb();
 if (new_process == NULL) 
 {
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
}
void syscallHandler(int excepCode){
// QUI VERIFICHIAMO SE E' in kernel mode
/*
important: In the implementation of each syscall, remember to access the global variables Process
Count, Ready Queue and Device Semaphores in a mutually exclusive way using ACQUIRE_LOCK and
RELEASE_LOCK on the Global Lock, in order to avoid race condition.
*/
state_t *stato = GET_EXCEPTION_STATE_PTR(excepCode);
int iskernel = stato->status & MSTATUS_MPP_M;

if (stato->reg_a0 < 0 && iskernel){
    // se il valore di a0 è negativo e siamo in kernel mode
    // allora dobbiamo fare un'azione descritta nella pagina 7 del pdf
   switch (stato->reg_a0)
   {
   case CREATEPROCESS:
    createProcess(stato);
    break;
   case TERMPROCESS:
    break;
   case PASSEREN:
    break;
   case VERHOGEN:
    break;
   case DOIO:
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
  stato->cause = PRIVINSTR; // perché non è possibile usare questi casi in usermode.
  programTrapHandler();
}
}


/*    
    }

    */



