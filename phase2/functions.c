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
      outProcQ(&pcbReady, process); // svuotiamo il processo dalla ready queue
      freePcb(process); // liberiamo il pcb nella free list
  
}
/* FUNZIONE PER CERCARE UN PROCESSO IN:
- CURRENT PROCESS
- READY QUEUE
- BLOCKED QUEUE
l'int remove serve per sapere se dobbiamo rimuovere il processo
e restituirlo, oppure no.
*/
pcb_t *findProcess(int pid, int remove) {
  for (int i = 0; i < NCPU; i++) {
    if (current_process[i] != NULL && current_process[i]->p_pid == pid) {
     if (remove) {
        // se remove è true, rimuoviamo il processo dalla current process
        pcb_t *processo = current_process[i];
        current_process[i] = NULL;
        process_count--;
        return processo;
      }
      return current_process[i];
    }
  }
  // sennò cerchiamo nella ready queue
  pcb_t *processo = NULL;
  list_for_each_entry(processo, &pcbReady, p_list) {
    if (processo->p_pid == pid) return processo;
 }
    // sennò cerchiamo nella blocked queue
  pcb_t *bloccato = findBlockedPid(pid, 1);
    return NULL; // non trovato
}
// implementazione della funzione memcpy che sennò dà problemi e dobbiamo impazzire
// Copia 'len' byte dall'area 'src' in 'dest'
// Ritorna il puntatore a 'dest'
// Questa funzione è simile a quella della libreria standard, ma non usa i registri
void* memcpy(void* dest, const void* src, unsigned int len) {
  char* d = dest;           // Cast destination pointer to a char pointer
  const char* s = src;      // Cast source pointer to a const char pointer
  while (len--) {           // Loop until 'len' bytes are copied
    *d++ = *s++;            // Copy each byte from source to destination
  }
  return dest;              // Return the destination pointer
}
int findDevice(memaddr* indirizzo_comando) {
  unsigned int offset = (unsigned int) indirizzo_comando - START_DEVREG;
  int i = -1;
  if (offset >= 32 * 0x10) {
    i = 32 + ((offset - (32 * 0x10)) / 0x8);
  } else {
    i = offset / 0x10;  
  }
  return i; 
}