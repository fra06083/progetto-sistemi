#include "./headers/pcb.h"
static struct list_head pcbFree_h; // sentinella della lista dei processi liberi o non utilizzati, NON NE FA PARTE. 
static pcb_t pcbFree_table[MAXPROC]; // tabella processi
static int next_pid = 1; // prossimo id del processo

/*

schema per fare l'inizializzazione
pcbfree_table[0] aggiunto a head.next...
pcbfree_table[1] aggiunto a head.next->next e mette head.next->prev = head.next...
....
*/

/* 
 initialize the pcbFree list to contain all
the elements of the static array of MAXPROC PCBs. This
method will be called only once during data structure
initialization.
 */
void initPcbs() {                                                                                                                        
   INIT_LIST_HEAD(&pcbFree_h);                                
   for (int i = 0; i<MAXPROC; i++){
       pcb_t *puntatore = &pcbFree_table[i]; // salvo il puntatore della lista, devo inizializzare per ogni i il list head
    freePcb(puntatore); // dava problemi con la vecchia configurazione, ora dà un pcb_t in input
  }
    }

/*
insert the element pointed to by p onto the pcbFree list.
*/
void freePcb(pcb_t* p) { // inserire l'elemento p nella lista pcbfree () nella coda ?? non chiede di fare nessuna condizione
    list_add_tail(&p->p_list, &pcbFree_h);
}
/*
return NULL if the pcbFree list is
empty. Otherwise, remove an element from the pcbFree list,
provide initial values for ALL of the PCBs fields and then
return a pointer to the removed element. PCBs get reused, so
it is important that no previous value persist in a PCB when it
gets reallocated.
*/
pcb_t* allocPcb() {
        if (list_empty(&pcbFree_h)) return NULL; // Return NULL if the pcbFree list is empty}
    else{
        struct list_head *primo_elemento = pcbFree_h.next;  //puntatore primo elemento, elimina il primo elemento FIFO
        list_del(primo_elemento);  
        pcb_t *pcb_rimosso = container_of(primo_elemento, pcb_t, p_list); 
// CONTAINER mi dà pcb_t della lista vista, non deve uscire null sennò è sbagliato.
        pcb_rimosso->p_parent = NULL;  
        INIT_LIST_HEAD(&pcb_rimosso->p_child);  
        INIT_LIST_HEAD(&pcb_rimosso->p_sib); 
        pcb_rimosso->p_supportStruct = NULL;  
        pcb_rimosso->p_pid = next_pid++;
        pcb_rimosso->p_time = 0;
        pcb_rimosso->p_semAdd = NULL;
        pcb_rimosso->p_s.cause = 0; // inizializzo il process state a NULL
        pcb_rimosso->p_s.status = 0; 
        pcb_rimosso->p_s.pc_epc = 0;
        pcb_rimosso->p_s.mie = 0; // inizializzo il process state a NULL
        pcb_rimosso->p_s.entry_hi = 0;  
        return (pcb_rimosso);
    }
}
/*
this
method is used to initialize a variable to be head pointer to a
process queue.
*/
void mkEmptyProcQ(struct list_head* head) {
    //Inizializzo una struct list_head (esistente) vuota 
    INIT_LIST_HEAD(head);     
}
/*
return TRUE
if the queue whose head is pointed to by head is empty.
Return FALSE otherwise.
*/
int emptyProcQ(struct list_head* head) {
    if(list_empty(head)) return TRUE;
    else return FALSE; 
}

/*
Insert the PCB pointed by p into the process queue whose head pointer is pointed to by head.
*/
void insertProcQ(struct list_head* head, pcb_t* p) {
    list_add_tail(&p->p_list, head); 
}

/*
Return a pointer to the first PCB from the process queue whose head is pointed to by head. Do
not remove this PCB from the process queue. Return NULL if the process queue is empty.
*/
pcb_t* headProcQ(struct list_head* head) {
    return container_of(list_next(head), pcb_t, p_list);
}

/*
Remove the first (i.e. head) element from the process queue whose head pointer is pointed to
by head. Return NULL if the process queue was initially empty; otherwise return the pointer
to the removed element.
*/
pcb_t* removeProcQ(struct list_head* head){
    if (list_empty(head)) return NULL;
    //Rimuovo il primo processo della process queue e faccio il return di quel pcb
    pcb_t *pcb = headProcQ(head);
    list_del(&pcb->p_list);
    return pcb;
}

/*
Remove the PCB pointed to by p from the process queue whose head pointer is pointed to by
head. If the desired entry is not in the indicated queue (an error condition), return NULL;
otherwise, return p. Note that p can point to any element of the process queue.
*/
pcb_t* outProcQ(struct list_head* head, pcb_t* p) { //(entry->s_procq, p)
    if (p == NULL) return NULL;
    // Scorro la lista per cercare il processo in input 
    struct list_head *lista_iter;
    list_for_each(lista_iter, head) {
        if (lista_iter == &p->p_list) {
            // processo trovato, rimuovilo dalla process queue
            list_del(lista_iter);
            return p; // Restituisco il PCB rimosso
        }
    }
    // pcb non trovato nella process queue
    return NULL;
}

int emptyChild(pcb_t* p) {
 // se la lista dei child del pcb p in input è vuota ritorna 1 altrimenti 0
   return list_empty(&p->p_child);
}

void insertChild(pcb_t* prnt, pcb_t* p) {
    //Se il processo padre non ha processi figli nel campo p_child, 
    if (emptyChild(prnt)) INIT_LIST_HEAD(&prnt->p_child); 
    p->p_parent = prnt; // metto il parent al puntatore
    list_add_tail(&p->p_sib, &prnt->p_child);
    /* qui ci va p->p_sib perché così li aggiunge come fratelli, 
    se prnt non ha fratelli inizializza la lista per poi aggiungere fratelli 
    la struttura è di processi, quindi ha senso che abbia un processo figlio che contiene una lista e i fratelli allo stesso livello
    */
}
pcb_t* removeChild(pcb_t* p) { 
    if (!emptyChild(p)){
        struct list_head *primo_nella_lista = p->p_child.next;
        list_del(primo_nella_lista);
        pcb_t *processo = container_of(primo_nella_lista, pcb_t, p_sib);
        processo->p_parent = NULL;
        return processo; 
    } 
    else return NULL;
}

//Make the PCB pointed to by p no longer the child of its parent. If the PCB pointed to by p has
//no parent, return NULL; otherwise, return p. Note that the element pointed to by p could be
//in an arbitrary position (i.e. not be the first child of its parent).
pcb_t* outChild(pcb_t* p) {
    if(p->p_parent == NULL) return NULL;
    // cancello il sibling e scollego il parent, p_child è il figlio, p_sibling i fratelli, quindi mi occorre controllare quello.
    list_del(&p->p_sib);
    p->p_parent = NULL; 
    return p;
}