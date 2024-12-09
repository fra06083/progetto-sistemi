#include "./headers/asl.h"

static semd_t semd_table[MAXPROC];
static struct list_head semdFree_h; // semafori non utilizzati
static struct list_head semd_h; // semafori attivi ASL LIST

/*

liste = semdFree_h.next
liste 
*/
void initASL() {

    INIT_LIST_HEAD(&semdFree_h); // inizializzo la testa
    for (int i = 0; i<MAXPROC;i++){
        semd_t *puntatore = &semd_table[i]; // qua prendo i semafori della tabella
        list_add_tail(&puntatore->s_link, &semdFree_h); // perché non sono ancora utilizzati

    }

}

int insertBlocked(int* semAdd, pcb_t* p) {
    // SE VUOTO INIZIALIZZIAMO, SE NON VUOTO AGGIUNGIAMO IN CODA.
    // if (list_empty(&semd_h)) INIT_LIST_HEAD(&semd_h);
    semd_t *semaforo = container_of(semAdd, semd_t, s_key);
    if(list_empty(&semaforo->s_procq)) INIT_LIST_HEAD(&semd_h);
    // list_add_tail(); aggiungere alla coda.
}

pcb_t* removeBlocked(int* semAdd) {
}

pcb_t* outBlockedPid(int pid) {
}

pcb_t* outBlocked(pcb_t* p) {
}

pcb_t* headBlocked(int* semAdd) {
}
