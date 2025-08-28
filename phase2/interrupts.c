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
        handleDeviceInterrupt(intlineNo, stato);
        break;
      default:
        break; // Nessuna gestione prevista
    }
}
void handleDeviceInterrupt(int intLine, state_t *stato){
    ACQUIRE_LOCK(&global_lock);  // Acquisizione lock globale
    // Non usiamo quello di sistema perché sappiamo già intLineNo, non eccezione
    memaddr devAddr = START_DEVREG + ((intLine - 3) * 0x80) + (getDevNo(intLine) * 0x10);
    unsigned int status = 0;
    pcb_t *unblocked = NULL;

    // Gestione per i terminali 
    if (intLine == 7) { 
        termreg_t *termReg = (termreg_t *)devAddr;
        unsigned int trans_stat = termReg->transm_status & 0xFF;
        unsigned int recv_stat = termReg->recv_status & 0xFF;
        int *semIO = NULL;

        // Se il terminale ha completato una trasmissione 
        if (trans_stat == 5) {  
            status = termReg->transm_status;
            termReg->transm_command = ACK;  // Acknowledge
            semIO = &sem[findDevice((memaddr *)&termReg->transm_command)];
        }
        // Se il terminale ha completato una ricezione
        else if (recv_stat == CHARRECV) {  
            status = termReg->recv_status;
            termReg->recv_command = ACK;  // Acknowledge
            semIO = &sem[findDevice((memaddr *)&termReg->recv_command)];
        }

        // Gestione semaforo associato al dispositivo terminale 
        if (semIO != NULL) {
            (*semIO)++;
            unblocked = removeBlocked(semIO);
        }

    } else {  
        // Gestione generica per dispositivi non terminali
        dtpreg_t *devReg = (dtpreg_t *)devAddr;
        status = devReg->status;  
        devReg->command = ACK;  

        int deviceID = findDevice((memaddr *)devAddr);
        int *semIO = &sem[deviceID];

        (*semIO)++;
        unblocked = removeBlocked(semIO);
    }

    // Ripristino del processo sbloccato
    if (unblocked != NULL) {
        unblocked->p_s.reg_a0 = status;
        insertProcQ(&pcbReady, unblocked);
    }

    // Ripristino esecuzione processo corrente o chiamata scheduler; terminiamo interrupt.
    int cpuid = getPRID();
    if (current_process[cpuid] != NULL) {
        RELEASE_LOCK(&global_lock);
        LDST(stato);
    } else {
        RELEASE_LOCK(&global_lock);
        scheduler();
    }
}


// Gestione del Process Local Timer (PLT) — Sezione 7.2
void handlePLTInterrupt(state_t *stato) {
    ACQUIRE_LOCK(&global_lock);
    int cpuid = getPRID();
    setTIMER(TIMESLICE * (*(cpu_t *)TIMESCALEADDR)); 
    // setTIMER(TIMESLICE);
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
    ACQUIRE_LOCK(&global_lock);
    // Sblocco dei processi in attesa del pseudo-clock — 7.3.2
    int *clock = &sem[PSEUDOSEM];
    pcb_t *bloccato = NULL;
    
    // rimuovo i processi bloccati e li inserisce nella ready queue
    //ACQUIRE LOCK era qua
    while ((bloccato = removeBlocked(clock)) != NULL) {
        insertProcQ(&pcbReady, bloccato);
    }
    //Vecchio RELEASE_LOCK

    // Ripristino dell’esecuzione del processo corrente 7.3.3
    if(current_process[getPRID()] != NULL) {
        RELEASE_LOCK(&global_lock);
        LDST(&current_process[getPRID()]->p_s);
    } else {
        RELEASE_LOCK(&global_lock);
        scheduler();
    }
}
