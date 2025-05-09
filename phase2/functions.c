#include "headers/functions.h"

void killProcess(pcb_t *process){
    // dobbiamo terminare il processo
    // e poi liberare la memoria
    if (process == NULL) return; // se il processo è NULL non facciamo niente, abbiamo già svuotato tutto
    while (!emptyChild(process)) {
      pcb_t* child = removeChild(process);
      killProcess(child); // chiamiamo ricorsivamente la funzione per uccidere i sottoprocessi
      /* Dobbiamo proprio vederlo come un albero, unico pcb_t che può avere la p_list e i p_child */
    }

    // PARTE CHE CANCELLA EFFETTIVAMENTE
    if (process->p_semAdd != NULL) {
      // Process blocked on semaphore
      outBlocked(process);
      (*process->p_semAdd)++;
    } else if (outProcQ(&pcbReady, process) == NULL) {
      // se il processo non è in ready queue
      // e non è in blocked queue, allora non lo troviamo
      // e non possiamo liberarlo
      return;
    }
    process_count--;
    if (process->p_parent != NULL) {
      outChild(process);  // rimuoviamo il processo dalla lista dei figli
    }
      outProcQ(&pcbReady, process); // svuotiamo il processo dalla ready queue
      freePcb(process); // liberiamo il pcb nella free list
      if (process == current_process[getPRID()])
        current_process[getPRID()] = NULL;

}
/* FUNZIONE PER CERCARE UN PROCESSO IN:
- CURRENT PROCESS
- READY QUEUE
- BLOCKED QUEUE
l'int remove serve per sapere se dobbiamo rimuovere il processo
e restituirlo, oppure no.
*/
pcb_t *findProcess(int pid) {
  for (int i = 0; i < NCPU; i++) {
    if (current_process[i] != NULL && current_process[i]->p_pid == pid) {
      return current_process[i];
    }
  }
  // sennò cerchiamo nella ready queue
  pcb_t *processo = NULL;
  list_for_each_entry(processo, &pcbReady, p_list) {
    if (processo->p_pid == pid) return processo;
 }
    // sennò cerchiamo nella blocked queue
  pcb_t *bloccato = findBlockedPid(pid);
  if (bloccato != NULL) {
    return bloccato; // trovato
  }
  return NULL; // non trovato
}
// implementazione della funzione memcpy che sennò dà problemi e dobbiamo impazzire
// Copia 'len' byte dall'area 'src' in 'dest'
// Ritorna il puntatore a 'dest'
// Questa funzione è simile a quella della libreria standard, ma non usa i registri
void* memcpy(void* dest, const void* src, unsigned int len) {
  char* d = dest;           
  const char* s = src;    
  while (len--) {           
    *d++ = *s++; // Copia byte per byte           
  }
  return dest;              
}
/* 

Field #    | Address          | Field Name
-------------------------------------------
   3       | (base) + 0xc     | DATA1
   2       | (base) + 0x8     | DATA1
   1       | (base) + 0x4     | COMMAND
   0       | (base) + 0x0     | STATUS

Da quanto ho capito in sostanza ogni device (guarda sezione 12, passa da 10 in 10 ed è esattamente quello che fa questa funzione)

*/
// funzione per trovare il dispositivo a partire dall'indirizzo di comando
int findDevice(memaddr* indirizzo_comando) { // dobbiamo trovare il dispositivo dal suo indirizzo
  unsigned int offset = (unsigned int) indirizzo_comando - START_DEVREG; // calcoliamo l'offset
  int dev = -1; // inizializziamo il dispositivo a -1
  /* Problema: come troviamo l'indice del dispositivo?
   SOLUZIONE:
   In sostanza abbiamo:
   32 dispositivi, 16 terminali, 2 sub-devices per terminale
   così ritorniamo l'indice del dispositivo in questo modo
  */
  if (offset >= (DEVICES * 0x10)) {
    dev = (DEVICES + ((offset - (DEVICES * 0x10)) / 0x8));
  } else {
    dev = (offset / 0x10);  
  }
  return dev; 
}
// calcolo del tempo, prende il tempo corrente e lo sottrae al tempo di inizio
cpu_t getTime(int p_id) {
  cpu_t ctime;
  STCK(ctime);
  cpu_t time = ctime - start_time[p_id]; // calcoliamo il tempo di esecuzione
  return time;
}

// funzione per bloccare un processo
void block(state_t *stato, int p_id, pcb_t* current){
/* funzione per non ripetere questa parte nelle varie syscall
   Così limitiamo il lock alle critical section
   e non lo teniamo bloccato per tutto il tempo
*/
  stato->pc_epc += 4;
  current->p_s = *stato;
  current->p_time += getTime(p_id);
  ACQUIRE_LOCK(&global_lock);
  current_process[p_id] = NULL;
  RELEASE_LOCK(&global_lock);
  scheduler();
}