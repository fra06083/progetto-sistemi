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
        break; // Nessuna gestione prevista
    }
}


// Gestione interrupt device I/O (inclusi terminali) — Sezione 7.1
void handleDeviceInterrupt(int intLine, int devNo) {
    int cpuid = getPRID();
    unsigned int status;
    //7.1.1 Calculate the address for this device’s device register [Section 12]
    memaddr devAddr = START_DEVREG + ((intLine - 3) * 0x80) + (devNo * 0x10);
    ACQUIRE_LOCK(&global_lock);
    // Gestione per i terminali 
    if (intLine == 7) { // se è 7 abbiamo la gestione di I/O per i terminali
        termreg_t *term_reg = (termreg_t *)devAddr;
        unsigned int status_tr = term_reg->transm_status & 0xFF;
        if (2 <= status_tr && status_tr <= OKCHARTRANS) { 
          status = term_reg->transm_status;
          term_reg->transm_command = ACK;
          devAddr += 0x8; // Abbiamo visto il calcolo dall'ultima tabella.
        } else {  
          status = term_reg->recv_status;
          term_reg->recv_command = ACK;
        }
      } else {
        // Gestione generica per dispositivi non terminali
        dtpreg_t *devReg = (dtpreg_t *)devAddr;
        status = devReg->status;  // Salvataggio dello stato del dispositivo
        devReg->command = ACK;  // Acknowledge scrivendo ACK nel registro command
      }
    
    // Gestione Semaforo - V operation su semaforo associato al device — Sezione 7.1.4
    int device = findDevice((memaddr *)devAddr); // trova il dispositivo
    int *semaforoV = &sem[device];  // get the semaphore of the device
    (*semaforoV)++;
    pcb_t *bloccato = removeBlocked(semaforoV); // rimuove il processo bloccato dal semaforo
    if (bloccato != NULL) {
        bloccato->p_s.reg_a0 = status;
        insertProcQ(&pcbReady, bloccato);
    }
    // Ripristino dell’esecuzione: processo corrente o invocazione dello scheduler — 7.1.7
    if(current_process[cpuid] != NULL) {
        RELEASE_LOCK(&global_lock);
        LDST(&current_process[cpuid]->p_s);
    } else {
        RELEASE_LOCK(&global_lock);
        scheduler();
    }
}


// Gestione del Process Local Timer (PLT) — Sezione 7.2
void handlePLTInterrupt(state_t *stato) {
    ACQUIRE_LOCK(&global_lock);
    int cpuid = getPRID();

    // Ricarica timer 
    //Acknowledge del PLT
   // setTIMER(TIMESLICE * (*(cpu_t *)TIMESCALEADDR)); non va
    setTIMER(TIMESLICE);
    //Calcolo tempo utilizzato
    current_process[cpuid]->p_time += getTime(cpuid);   // calcoliamo il tempo di esecuzione
    current_process[cpuid]->p_s = *stato;               // copiamo lo stato del processo corrente
    insertProcQ(&pcbReady, current_process[cpuid]);     // mettiamo il processo corrente nella ready queue
    current_process[getPRID()] = NULL;
    RELEASE_LOCK(&global_lock);
    scheduler();     // Selezione di un nuovo processo da eseguire
}


// Gestione Interval Timer (Sezione 7.3)
//dovrebbe essere tutto ok
void handleIntervalTimerInterrupt() {
    // Ricarica timer
    //7.3.1  Acknowledgement dell'interrupt e ricarica dell'Interval Timer
    LDIT(PSECOND);

    // Sblocco dei processi in attesa del pseudo-clock — 7.3.2
    int *clock = &sem[PSEUDOSEM];
    pcb_t *bloccato = NULL;
    
    // rimuovo i processi bloccati e li inserisce nella ready queue
    ACQUIRE_LOCK(&global_lock);
    while ((bloccato = removeBlocked(clock)) != NULL) {
        insertProcQ(&pcbReady, bloccato);
    }
    RELEASE_LOCK(&global_lock);
    // RILASCIAMO IL LOCK GLOBALE

    // Ripristino dell’esecuzione del processo corrente 7.3.3
    if(current_process[getPRID()] != NULL) {
        LDST(&current_process[getPRID()]->p_s);
    } else {
        scheduler();
    }
}
