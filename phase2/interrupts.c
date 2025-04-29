#include "headers/interrupts.h"


// Interrupt Handler Principale
void interruptHandler(int excCode) {

    unsigned int cause = getCAUSE();

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
// problema registro a0
void handleDeviceInterrupt(int intLine, int devNo) {
    ACQUIRE_LOCK(&global_lock);
    int cpuid = getPRID();

    // Calcolo indirizzo dispositivo
    //7.1.1 Calculate the address for this device’s device register [Section 12]
    //coniglia di utilizzare uno switch per gestire i devNo, ma noi lo riceviamo già passato
    memaddr devAddr = START_DEVREG + ((intLine - 3) * 0x80) + (devNo * 0x10);
    
    // Lettura status e ACK
    //7.1.2 Save off the status code from the (sub)device’s device register
    //viene salvato ma non ancora nella pcb
    unsigned int status = *((unsigned int*)devAddr);
    //7.1.3 Acknowledge l'interrupt (scrivere ACK nel command register):
    *((unsigned int*)(devAddr + 0x4)) = ACK;

    // Gestione semaforo
    //7.1.4 V operation su semaforo associato al device
    semd_t *semd = findSemaphore((int*)devAddr);
    if(semd != NULL && !list_empty(&semd->s_procq)) {
        pcb_t *pcb = container_of(semd->s_procq.next, pcb_t, p_list);
        list_del(&pcb->p_list);
        pcb->p_semAdd = NULL;
        //7.1.5 Salvare il valore di status nel registro a0 del PCB sbloccato
        //pcb->p_s.reg_a0 = status; // Stato nel registro a0 è Sbagliato, dobbiamo convertire, contrgollate
        
        // Inserimento in ready queue
        //7.1.6 Inserire il PCB nella ready queue
        list_add_tail(&pcb->p_list, &pcbReady);
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
void handlePLTInterrupt() {
    ACQUIRE_LOCK(&global_lock);
    int cpuid = getPRID();

    // Ricarica timer 
    //Acknowledge del PLT
    setTIMER(TIMESLICE);

    // Aggiornamento processo corrente
    if(current_process[cpuid] != NULL) {
        //Salvare il processor state nel PCB (p_s)
      // NON ESISTE  save_state(&(current_process[cpuid]->p_s));

        //Calcolo tempo utilizzato
        cpu_t endTime;
        STCK(endTime);

        //Aggiornamento del tempo del processo con il tempo utilizzato
        cpu_t elapsed = endTime - start_time[cpuid];
        current_process[cpuid]->p_time += elapsed; 

        // Reinserimento in ready queue
        //Inserire nella Ready Queue:
        list_add_tail(&current_process[cpuid]->p_list, &pcbReady);

        // Svuota lo slot del processo corrente
        current_process[cpuid] = NULL;
    }

    RELEASE_LOCK(&global_lock);
    scheduler();
}


// Gestione Interval Timer (Sezione 7.3)
//dovrebbe essere tutto ok
void handleIntervalTimerInterrupt() {
    ACQUIRE_LOCK(&global_lock);

    // Ricarica timer
    //7.3.1  Acknowledgement dell'interrupt e ricarica dell'Interval Timer
    LDIT(PSECOND);

    // Sblocco processi pseudo-clock (sem[NSEMAPHORES-1])
    //7.3.2 Unblock all PCBs blocked waiting a Pseudo-clock tick
    int *clock = &sem[NRSEMAPHORES - 1];
    semd_t *clockSem = findSemaphore(clock);
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
    //7.3.3 Return control to the Current Process of the current CPU
    if(current_process[getPRID()] != NULL) {
        LDST(&current_process[getPRID()]->p_s);
    } else {
        scheduler();
    }
}
