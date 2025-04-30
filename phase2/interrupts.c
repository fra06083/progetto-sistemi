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
int getIntLineNo(int devNo)
{
    switch (devNo) {
        case 0: return DEV0ON;
        case 1: return DEV1ON;
        case 2: return DEV2ON;
        case 3: return DEV3ON;
        case 4: return DEV4ON;
        case 5: return DEV5ON;
        case 6: return DEV6ON;
        case 7: return DEV7ON;
        default: return -1; // DevNo non valido
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
        handleDeviceInterrupt(intlineNo, getDevNo(intCode));
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
    //coniglia di utilizzare uno switch per gestire i devNo, ma noi lo riceviamo già passato
    memaddr devAddr = START_DEVREG + ((intLine - 3) * 0x80) + (devNo * 0x10);
    ACQUIRE_LOCK(&global_lock);
    // MANCAVA STA PARTE
    if (intLine == 7) {
        /* Terminal Interrupt */
        termreg_t *termReg = (termreg_t *)devAddr;
        unsigned int trans_stat = termReg->transm_status & 0xFF;
        if (2 <= trans_stat && trans_stat <= OKCHARTRANS) { 
          status = termReg->transm_status;
          termReg->transm_command = ACK;
          devAddr += 0x8;
        } else {  // Int is recv
          status = termReg->recv_status;
          termReg->recv_command = ACK;
        }
      } else {
        /* Generic device Interrupt */
        dtpreg_t *devReg = (dtpreg_t *)devAddr;
        status = devReg->status;  // punto 2
        devReg->command = ACK;
      }
    
    //7.1.3 Acknowledge l'interrupt (scrivere ACK nel command register):
    *((unsigned int*)(devAddr + 0x4)) = ACK;

    // Gestione semaforo
    //7.1.4 V operation su semaforo associato al device
    int *semaforoV = &sem[devNo];  // get the semaphore of the device
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
    cpu_t endTime;
    STCK(endTime);

    //Aggiornamento del tempo del processo con il tempo utilizzato
    cpu_t elapsed = endTime - start_time[cpuid];
    current_process[cpuid]->p_time += elapsed; 
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
