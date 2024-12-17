#include "./headers/pcb.h"
static struct list_head pcbFree_h; // sentinella della lista dei processi liberi o non utilizzati, NON NE FA PARTE. 
static pcb_t pcbFree_table[MAXPROC]; // tabella processi
static int next_pid = 1; // prossimo id del processo

/*

schema
pcbfree_table[0] aggiunto a head.next...
pcbfree_table[1] aggiunto a head.next->next e mette head.next->prev = head.next...
....
*/
void initPcbs() {                                                                                                                        
   INIT_LIST_HEAD(&pcbFree_h);                                
   for (int i = 0; i<MAXPROC; i++){
       pcb_t *puntatore = &pcbFree_table[i]; // salvo il puntatore della lista, devo inizializzare per ogni i il list head
    freePcb(puntatore); // dava problemi con la vecchia configurazione, ora dà un pcb_t in input
  }
    }

void freePcb(pcb_t* p) { // inserire l'elemento p nella lista pcbfree () nella coda ?? non chiede di fare nessuna condizione
    list_add_tail(&p->p_list, &pcbFree_h);
}

pcb_t* allocPcb() {
        if (list_empty(&pcbFree_h)) return NULL; // Return NULL if the pcbFree list is empty}
    else{
        struct list_head *primo_elemento = pcbFree_h.next;  //puntatore primo elemento, elimina il primo elemento FIFO
        list_del(primo_elemento);  
        pcb_t *pcb_rimossso = container_of(primo_elemento, pcb_t, p_list); 
// CONTAINER mi dà pcb_t della lista vista, non deve uscire null sennò è sbagliato.
        pcb_rimossso->p_parent = NULL;  
        INIT_LIST_HEAD(&pcb_rimossso->p_child);  
        INIT_LIST_HEAD(&pcb_rimossso->p_sib); 
        pcb_rimossso->p_supportStruct = NULL;  
        pcb_rimossso->p_pid = next_pid;  
        pcb_rimossso->p_time = 0;
        pcb_rimossso->p_semAdd = NULL;
        return (pcb_rimossso);
    }
}

void mkEmptyProcQ(struct list_head* head) { //head è il puntatore alla testa della lista (vuota) che verrà riempita coi PCB's 
    //Inizializzo una struct list_head (esistente) vuota 
    INIT_LIST_HEAD(head);     
}

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
    pcb_t *pcb = headProcQ(head);
    list_del(&pcb->p_list);
   //Rimuovi il nodo dalla lista
    return pcb;
}

/*
Remove the PCB pointed to by p from the process queue whose head pointer is pointed to by
head. If the desired entry is not in the indicated queue (an error condition), return NULL;
otherwise, return p. Note that p can point to any element of the process queue.
*/
pcb_t* outProcQ(struct list_head* head, pcb_t* p) { //(entry->s_procq, p)
    if (p == NULL) return NULL;
    // Scorri la lista per cercare il nodo p
    struct list_head *lista_iter;
    list_for_each(lista_iter, head) {
        if (lista_iter == &p->p_list) {
            // Nodo trovato, rimuovilo dalla lista
            list_del(lista_iter);
            return p; // Restituisci il PCB rimosso
        }
    }
    // Nodo non trovato nella coda
    return NULL;
}

int emptyChild(pcb_t* p) { // non so se ha senso sinceramente da cambiare e confermare
   //struct list_head* child = &p->p_child;
   //return (list_empty(child));
   return list_empty(&p->p_child);
}

void insertChild(pcb_t* prnt, pcb_t* p) {
    if (emptyChild(prnt)) INIT_LIST_HEAD(&prnt->p_child); 
    p->p_parent = prnt; // metto il parent al puntatore
    list_add_tail(&p->p_sib, &prnt->p_child); 
    /* qui ci va p->p_sib perché così li aggiunge come fratelli, se prnt non ha fratelli inizializza la lista per poi aggiungere fratelli 
    la struttura è di processi, quindi ha senso che abbia un processo figlio che contiene una lista e i fratelli allo stesso livello
    */
}

pcb_t* removeChild(pcb_t* p) { 
    if (!emptyChild(p)){
        struct list_head *primo_nella_lista = p->p_child.next;
        list_del(primo_nella_lista);
        pcb_t *processo = container_of(primo_nella_lista, pcb_t, p_sib);
        processo->p_parent = NULL;
        return processo; // qua mandiamo in output il processo eliminato, prima avevamo scritto p in modo sbagliato
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