#include "headers/interrupts.h"
int getDevNo(int IntlineNo)
{
    memaddr bit_map = BITMAP(IntlineNo);

    if      (bit_map & DEV0ON) return 0;
    else if (bit_map & DEV1ON) return 1;
    else if (bit_map & DEV2ON) return 2;
    else if (bit_map & DEV3ON) return 3;
    else if (bit_map & DEV4ON) return 4;
    else if (bit_map & DEV5ON) return 5;
    else if (bit_map & DEV6ON) return 6;
    else if (bit_map & DEV7ON) return 7;
    else                      return -1; // Nessun dispositivo trovato
}
int getIntLineNo(int intCode)
{
  switch (intCode) {
    case IL_CPUTIMER:
      return 1;
    case IL_TIMER:
      return 2;
    case IL_DISK:
      return 3;
    case IL_FLASH:
      return 4;
    case IL_ETHERNET:
      return 5;
    case IL_PRINTER:
      return 6;
    case IL_TERMINAL:
      return 7;
    default:
      return -1;
  }
}
// Interrupt Handler Principale
void interruptHandler(int intCode, state_t *stato) {
    int intlineNo = getIntLineNo(intCode);

    switch (intlineNo) {
      case 1:
        handlePLTInterrupt(stato);
        break;
      case 2:
        handleIntervalTimerInterrupt();
        break;
      case 3 ... 7:
        handleDeviceInterrupt(intlineNo, getDevNo(intlineNo));
        break;
      default:
        break;
    }
}


// Gestione Interrupt Dispositivo (Sezione 7.1)
// problema registro a0
void handleDeviceInterrupt(int intLine, int devNo) {
    int cpuid = getPRID();
    unsigned int status;
    // Calcolo indirizzo dispositivo
    //7.1.1 Calculate the address for this device’s device register [Section 12]
    memaddr devAddr = START_DEVREG + ((intLine - 3) * 0x80) + (devNo * 0x10);
    ACQUIRE_LOCK(&global_lock);
    // MANCAVA STA PARTE
    if (intLine == 7) { // gestione terminale
        termreg_t *termReg = (termreg_t *)devAddr;
        unsigned int trans_stat = termReg->transm_status & 0xFF;
        if (2 <= trans_stat && trans_stat <= OKCHARTRANS) { 
          status = termReg->transm_status;
          termReg->transm_command = ACK;
          devAddr += 0x8; // Abbiamo visto il calcolo dall'ultima tabella.
        } else {  
          status = termReg->recv_status;
          termReg->recv_command = ACK;
        }
      } else {
        // device generico, se non è un terminale
        dtpreg_t *devReg = (dtpreg_t *)devAddr;
        status = devReg->status;  // punto 7.2, ci salviamo lo stato del dispositivo
        devReg->command = ACK; //7.1.3 Acknowledge l'interrupt (scrivere ACK nel command register):
      }
    
    // Gestione semaforo
    //7.1.4 V operation su semaforo associato al device
    int device = findDevice((memaddr *)devAddr); // trova il dispositivo
    int *semaforoV = &sem[device];  // get the semaphore of the device
    (*semaforoV)++;
    pcb_t *bloccato = removeBlocked(semaforoV); // rimuove il processo bloccato dal semaforo
    if (bloccato != NULL) {
        bloccato->p_semAdd = NULL;
        bloccato->p_s.reg_a0 = status;
        insertProcQ(&pcbReady, bloccato);
    }
    RELEASE_LOCK(&global_lock);
    // Ripristino esecuzione
    //7.1.7 Tornare al Current Process o Scheduler:
    if(current_process[cpuid] != NULL) {
        LDST(&current_process[cpuid]->p_s);
    } else {
        scheduler();
    }
}


// Gestione PLT (Sezione 7.2)
//forse problema con tempo processo
void handlePLTInterrupt(state_t *stato) {
    ACQUIRE_LOCK(&global_lock);
    int cpuid = getPRID();

    // Ricarica timer 
    //Acknowledge del PLT
    setTIMER(TIMESLICE);
    //Calcolo tempo utilizzato
    current_process[cpuid]->p_time += getTime(cpuid); // calcoliamo il tempo di esecuzione
    current_process[cpuid]->p_s = *stato;       // copiamo lo stato del processo corrente
    insertProcQ(&pcbReady, current_process[cpuid]);  // mettiamo il processo corrente nella ready queue
    current_process[getPRID()] = NULL;
    RELEASE_LOCK(&global_lock);
    scheduler();
}


// Gestione Interval Timer (Sezione 7.3)
//dovrebbe essere tutto ok
void handleIntervalTimerInterrupt() {
    // Ricarica timer
    //7.3.1  Acknowledgement dell'interrupt e ricarica dell'Interval Timer
    LDIT(PSECOND);

    // Sblocco processi pseudo-clock (sem[NSEMAPHORES-1])
    //7.3.2 Unblock all PCBs blocked waiting a Pseudo-clock tick
    int *clock = &sem[PSEUDOSEM];
    pcb_t *bloccato; // serve solo per il while
    // rimuovo i processi bloccati
    // e li inserisce nella ready queue
    ACQUIRE_LOCK(&global_lock);
    while ((bloccato = removeBlocked(clock)) != NULL) {
        insertProcQ(&pcbReady, bloccato);
    }
    RELEASE_LOCK(&global_lock);
    // RILASCIAMO IL LOCK GLOBALE qui

    // Ripristino esecuzione
    //7.3.3 Return control to the Current Process of the current CPU
    if(current_process[getPRID()] != NULL) {
        LDST(&current_process[getPRID()]->p_s);
    } else {
        scheduler();
    }
}
