#include "./headers/exceptions.h"
#include "/usr/include/uriscv/types.h"
/*

Important: To determine if the Current Process was executing in kernel-mode or user-mode,
one examines the Status register in the saved exception state. In particular, examine the MPP field
if the status register with saved_exception_state->status & MSTATUS_MPP_MASK see section Dott.
Rovelli’s thesis for more details.

*/

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
  new_process->p_s = *(state_t *) c_state->reg_a1;
  new_process->p_supportStruct = (support_t *) c_state->reg_a3;
  insertProcQ(&current_process[getPRID()]->p_list, &new_process);
  insertChild(&current_process[getPRID()], &new_process->p_child);
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

void uTLB_ExceptionHandler(){


}
void programTrapHandler(){

}
/*    
    }

    */




/*
// SPDX-FileCopyrightText: 2004 Mauro Morsiani
// SPDX-FileCopyrightText: 2020 Mikey Goldweber
// SPDX-FileCopyrightText: 2023 Gianmaria Rovelli
//
// SPDX-License-Identifier: GPL-3.0-or-later

/****************************************************************************
 *
 * This header file contains utility types definitions.
 *
 ****************************************************************************

 #ifndef URISCV_TYPES_H
 #define URISCV_TYPES_H
 
 /* Device register type for disks, flash devices and printers (dtp) 
 typedef struct dtpreg {
   unsigned int status;
   unsigned int command;
   unsigned int data0;
   unsigned int data1;
 } dtpreg_t;
 
 /* Device register type for terminals 
 typedef struct termreg {
   unsigned int recv_status;
   unsigned int recv_command;
   unsigned int transm_status;
   unsigned int transm_command;
 } termreg_t;
 
 typedef union devreg {
   dtpreg_t dtp;
   termreg_t term;
 } devreg_t;
 
 /* Bus register area 
 typedef struct devregarea {
   unsigned int rambase;
   unsigned int ramsize;
   unsigned int execbase;
   unsigned int execsize;
   unsigned int bootbase;
   unsigned int bootsize;
   unsigned int todhi;
   unsigned int todlo;
   unsigned int intervaltimer;
   unsigned int timescale;
   unsigned int tlb_floor_addr;
   unsigned int inst_dev[5];
   unsigned int interrupt_dev[5];
   devreg_t devreg[5][8];
 } devregarea_t;
 
 /* Pass Up Vector 
 typedef struct passupvector {
   unsigned int tlb_refill_handler;
   unsigned int tlb_refill_stackPtr;
   unsigned int exception_handler;
   unsigned int exception_stackPtr;
 } passupvector_t;
 
 #define STATE_GPR_LEN 32
 
 /* Processor state 
 typedef struct state {
   unsigned int entry_hi;
   unsigned int cause;
   unsigned int status;
   unsigned int pc_epc;
   unsigned int mie;
   unsigned int gpr[STATE_GPR_LEN];
 } state_t;
 
 #define reg_zero gpr[0]
 #define reg_ra gpr[1]
 #define reg_sp gpr[2]
 #define reg_gp gpr[3]
 #define reg_tp gpr[4]
 #define reg_t0 gpr[5]
 #define reg_t1 gpr[6]
 #define reg_t2 gpr[7]
 #define reg_t3 gpr[8]
 #define reg_t4 gpr[9]
 #define reg_t5 gpr[10]
 #define reg_t6 gpr[11]
 #define reg_s0 gpr[12]
 #define reg_s1 gpr[13]
 #define reg_s2 gpr[14]
 #define reg_s3 gpr[15]
 #define reg_s4 gpr[16]
 #define reg_s5 gpr[17]
 #define reg_s6 gpr[18]
 #define reg_s7 gpr[19]
 #define reg_s8 gpr[20]
 #define reg_s9 gpr[21]
 #define reg_s10 gpr[22]
 #define reg_s11 gpr[23]
 #define reg_a0 gpr[24]
 #define reg_a1 gpr[25]
 #define reg_a2 gpr[26]
 #define reg_a3 gpr[27]
 #define reg_a4 gpr[28]
 #define reg_a5 gpr[29]
 #define reg_a6 gpr[30]
 #define reg_a7 gpr[31]
 
 #define ETH_ALEN 6
 #define ETH_PAYLOAD 1500
 
 typedef struct packet {
   unsigned char dest[ETH_ALEN];
   unsigned char src[ETH_ALEN];
   unsigned char proto[2];
   unsigned char data[ETH_PAYLOAD];
   unsigned char dummy[2];
 } packet_t;
 
 #endif // URISV_TYPES_H
*/