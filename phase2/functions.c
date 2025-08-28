#include "headers/functions.h"

void killProcess(pcb_t * process){      // dobbiamo terminare il processo, poi liberare la memoria

    if (process == NULL) return; // se il processo è NULL non facciamo niente, già svuotato
    while (!emptyChild(process)) {
        pcb_t * child = removeChild(process);
        killProcess(child); // chiamiamo ricorsivamente la funzione per uccidere i sottoprocessi
                            // Visto come un albero, unico pcb_t che può avere la p_list e i p_child
    }

    // Rimozione effettiva del processo
    if (process->p_semAdd != NULL) {
        // Processo bloccato su semaforo: rimuovo e incremento il semaforo
        outBlocked(process);
    } else if (outProcQ(&pcbReady, process) == NULL) {
        // Se il processo non è in ready queue e non è in blocked queue --> non gestibile
        return;
    }
    process_count--;
    if (process->p_parent != NULL) {
        outChild(process);  // Rimuoviamo il processo dalla lista dei figli
    }
    outProcQ(&pcbReady, process); // Rimozione dalla ready queue
    freePcb(process); // Liberiamo il pcb nella free list
    if (process == current_process[getPRID()])
        current_process[getPRID()] = NULL;

}
/* Funzione che cerca il processo in:
- current process
- ready queue
- blocked queue
*/
pcb_t * findProcess(int pid) {
    for (int i = 0; i < NCPU; i++) {
        if (current_process[i] != NULL && current_process[i] -> p_pid == pid) {
            return current_process[i];
        }
    }
    //check ready queue
    pcb_t * processo = NULL;
    list_for_each_entry(processo, & pcbReady, p_list) {
        if (processo -> p_pid == pid) return processo;
    }
    // check blocked queue
    pcb_t * bloccato = findBlockedPid(pid);
    if (bloccato != NULL) {
        return bloccato; // found
    }
    return NULL; // not found
}
/* implementazione della funzione memcpy che sennò dà problemi e dobbiamo impazzire
 * Copia 'len' byte dall'area 'src' in 'dest'
 * Ritorna il puntatore a 'dest'
 * Questa funzione è simile a quella della libreria standard, ma non usa i registri
 */
void *memcpy(void *dest, const void *src, unsigned int len)
{
    char *d = dest;         // Puntatore alla destinazione, byte per byte
    const char *s = src;    // Puntatore alla sorgente, byte per byte
    while (len--) {         // Finché ci sono byte da copiare
        *d++ = *s++;        // Copia il byte corrente e avanza entrambi i puntatori
    }
    return dest;            // Ritorna il puntatore originale alla destinazione
}

/* 

Field #    | Address          | Field Name
-------------------------------------------
   3       | (base) + 0xc     | DATA1
   2       | (base) + 0x8     | DATA1
   1       | (base) + 0x4     | COMMAND
   0       | (base) + 0x0     | STATUS

Ogni device ha un layout a 16 byte (0x10), tranne i terminali che usano 8 byte.
Il calcolo dell’indice considera:
- 32 dispositivi generici (ogni 0x10 byte)
- 16 terminali con 2 sub-dispositivi (ogni 0x8 byte)

*/
/*
 *    'register' è un hint per il compilatore: indica che la variabile 
 *    può essere memorizzata in un registro della CPU invece che in RAM!
 *    può essere utile per garantire l’uso dei registri.
 *    Viene usato un puntatore di tipo unsigned char* per garantire che
 *    si scrivano byte singoli indipendentemente dal tipo di memoria originale.
 *    register is a hint to the compiler, advising it to store 
 *    that variable in a processor register instead of memory 
 *    (for example, instead of the stack).
 */
// Funzione che trova il dispositivo a partire dall'indirizzo di comando
void *memset(void *dest, register int val, register unsigned int len) {
    register unsigned char *ptr = (unsigned char*)dest; // puntatore che scorre i byte

    while (len-- > 0) {
        *ptr++ = val; // scrive il byte e avanza il puntatore
    }

    return dest; // ritorna il puntatore originale
}
int findDevice(memaddr * indirizzo_comando) {
  unsigned int getDevices = (unsigned int) indirizzo_comando - START_DEVREG; // togliamo dall'indirizzo il base address
  /* Problema: come troviamo l'indice del semaforo da bloccare?
   SOLUZIONE:
   In sostanza abbiamo:
   32 dispositivi, 8 terminali (2 sub-devices per terminale quindi 16 indici) + 1 PSEUDOCLOCK non contato in NSUPPSEM
   noi abbiamo pensato che riserviamo i primi 32 indici per i dispositivi generici
   e i successivi 16 per i terminali poi ritorniamo l'indice del dispositivo
  */
  int indice = -1;
  if (getDevices >= (DEVICES * 0x10)) { // allora è un terminale
    indice = (DEVICES + ((getDevices - (DEVICES * 0x10)) / 0x8));
  } else {
    indice = (getDevices / 0x10);
  }
  if (indice == -1 || indice >= NSUPPSEM) {
    return -1; // errore, dispositivo non valido
  }
  return indice;
}
// calcolo del tempo, prende il tempo corrente e lo sottrae al tempo di inizio
cpu_t getTime(int p_id) {
  cpu_t ctime;
  STCK(ctime);
  cpu_t time = ctime - start_time[p_id];// calcolo effettivo del tempo
  return time;
}

// Funziona che blocca un processo: aggiorna lo stato, salva il tempo e chiama scheduler
// Funzione per non ripetere questa parte nelle varie syscall
// Così limitiamo il lock alle critical section
// e non lo teniamo bloccato per tutto il tempo

void block(state_t * stato, int p_id, pcb_t * current){
    stato->pc_epc += 4;
    current->p_s = *stato;
    // Aggiorniamo il tempo di CPU usato prima di bloccare
    current->p_time += getTime(p_id);
    current_process[p_id] = NULL;
}
