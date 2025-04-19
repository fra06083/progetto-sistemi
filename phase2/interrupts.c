#include "./headers/interrupts.h"




// Funzione di supporto per trovare semaforo (implementata in ASL)
extern semd_t* findSemaphore(int *semAddr); // non esiste, la metteremo noi in functions.c


// Interrupt Handler Principale
void interruptHandler() {
    unsigned int cause = getCAUSE();
    int excCode = cause & CAUSE_EXCCODE_MASK;

    if (excCode == IL_CPUTIMER) {
        handlePLTInterrupt();
    } else if (excCode == IL_TIMER) {
        handleIntervalTimerInterrupt();
    } else {
        // Scansione linee di interrupt 3-7 (dispositivi)
        for(int intLine = 3; intLine <= 7; intLine++) {
            memaddr bitmapAddr = 0x10000040 + (intLine - 3) * 4;
            unsigned int bitmap = *((unsigned int*)bitmapAddr);
            
            if(bitmap != 0) {
                for(int devNo = 0; devNo < 8; devNo++) {
                    if(bitmap & (1 << devNo)) {
                        handleDeviceInterrupt(intLine, devNo);
                        return;
                    }
                }
            }
        }
    }
}


// Gestione Interrupt Dispositivo (Sezione 7.1)
void handleDeviceInterrupt(int intLine, int devNo) {
    ACQUIRE_LOCK(&global_lock);
    int cpuid = getPRID();

    // Calcolo indirizzo dispositivo
    memaddr devAddr = START_DEVREG + ((intLine - 3) * 0x80) + (devNo * 0x10);
    
    // Lettura status e ACK
    unsigned int status = *((unsigned int*)devAddr);
    *((unsigned int*)(devAddr + 0x4)) = ACK;

    // Gestione semaforo
    semd_t *semd = findSemaphore((int*)devAddr);
    if(semd != NULL && !list_empty(&semd->s_procq)) {
        pcb_t *pcb = container_of(semd->s_procq.next, pcb_t, p_list);
        list_del(&pcb->p_list);
        pcb->p_semAdd = NULL;
 //       pcb->p_s.reg_a0 = status; // Stato nel registro a0 E' Sbagliato, dobbiamo convertire, contrgollate
        
        // Inserimento in ready queue
        list_add_tail(&pcb->p_list, &pcbReady);
    }

    RELEASE_LOCK(&global_lock);

    // Ripristino esecuzione
    if(current_process[cpuid] != NULL) {
        LDST(&current_process[cpuid]->p_s);
    } else {
        scheduler();
    }
}


// Gestione PLT (Sezione 7.2)
void handlePLTInterrupt() {
    ACQUIRE_LOCK(&global_lock);
    int cpuid = getPRID();

    // Ricarica timer
    setTIMER(TIMESLICE);

    // Aggiornamento processo corrente
    if(current_process[cpuid] != NULL) {
        // Calcolo tempo utilizzato
        cpu_t endTime;
        STCK(endTime);
   // WRONG!!!!     current_process[cpuid]->p_time += (endTime - current_process[cpuid]->p_s);
        
        // Reinserimento in ready queue
        list_add_tail(&current_process[cpuid]->p_list, &pcbReady);
        current_process[cpuid] = NULL;
    }

    RELEASE_LOCK(&global_lock);
    scheduler();
}


// Gestione Interval Timer (Sezione 7.3)
void handleIntervalTimerInterrupt() {
    ACQUIRE_LOCK(&global_lock);

    // Ricarica timer
    LDIT(PSECOND);

    // Sblocco processi pseudo-clock (sem[NSEMAPHORES-1])
    semd_t *clockSem = &sem[NRSEMAPHORES - 1];
    if(!list_empty(&clockSem->s_procq)) {
        struct list_head *pos;
        list_for_each(pos, &clockSem->s_procq) {
            pcb_t *pcb = container_of(pos, pcb_t, p_list);
            list_del(pos);
            list_add_tail(pos, &pcbReady);
        }
    }

    RELEASE_LOCK(&global_lock);

    // Ripristino esecuzione
    if(current_process[getPRID()] != NULL) {
        LDST(&current_process[getPRID()]->p_s);
    } else {
        scheduler();
    }
}