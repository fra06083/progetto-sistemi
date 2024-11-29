#include "./headers/pcb.h"

static struct list_head pcbFree_h; // sentinella della lista dei processi liberi o non utilizzati, NON NE FA PARTE.
static pcb_t pcbFree_table[MAXPROC]; // tabella processi
static int next_pid = 1; // prossimo id del processo

/*

schema
pcbfree_table[0] aggiunto a head.next...
pcbfree_table[1] aggiunto a head.next->next e mette head.next->prev = head.next...
....
passaggio finale SENTINELLA.prev = ultimo della lista

*/
void initPcbs() {
struct list_head *lista = &pcbFree_h.next;
/* essendo la sentinella non un puntatore 
ma una struttura inizializzo .next per poi prev = ultimo elemento salvato
*/
 INIT_LIST_HEAD(&lista);
for (int i = 0; i <MAXPROC; i++){
            list_add(&pcbFree_table[i], lista); // agggiunge per pcbFree_table la lista corrispettiva nella lista.
        }
 pcbFree_h.prev = &lista;
}

void freePcb(pcb_t* p) { // inserire l'elemento p nella lista pcbfree () nella coda ?? non chiede di fare nessuna condizione

   list_add_tail(&p, &pcbFree_h);

}

pcb_t* allocPcb() {
   if (list_empty(&pcbFree_h)) return NULL; // rimuovo quello inserito per prima e poi dopo quelli dopo (FIFO)
   pcb_t* pcb_rimosso = &pcbFree_h.next;
   list_del(&pcbFree_h.next);
   INIT_LIST_HEAD(pcb_rimosso);
   return pcb_rimosso;
}

void mkEmptyProcQ(struct list_head* head) {
}

int emptyProcQ(struct list_head* head) {
}

void insertProcQ(struct list_head* head, pcb_t* p) {
}

pcb_t* headProcQ(struct list_head* head) {
}

pcb_t* removeProcQ(struct list_head* head) {
}

pcb_t* outProcQ(struct list_head* head, pcb_t* p) {
}

int emptyChild(pcb_t* p) { // non so se ha senso sinceramente da cambiare e confermare
   struct list_head* child = &p->p_child;
   return (list_empty(child));
}

void insertChild(pcb_t* prnt, pcb_t* p) {
}

pcb_t* removeChild(pcb_t* p) {
}

pcb_t* outChild(pcb_t* p) {
}


/* 
#include "headers/pcb.h"

static pcb_t pcbTable[MAXPROC];  // PCB array with maximum size 'MAXPROC' 
LIST_HEAD(pcbFree_h);            // List of free PCBs                     
static int next_pid = 1;


//Initialize the pcbFree list to contain all the elements of the static array of MAXPROC PCBs.
//This method will be called only once during data structure initialization.

void initPcbs() {
    for(int i=0; i<MAXPROC; i++){ 
        pcb_t *new_pcb = &pcbTable[i];      //crea pointer
        INIT_LIST_HEAD(&new_pcb->p_list);   //Funzione che inizializza la lista list come vuota
        freePcb(new_pcb);   //insert element into pcbFree list
    }
}


//Insert the element pointed to by p onto the pcbFree list.
//Come list_add, ma inserisce l'elemento new in coda
//  new: nuovo elemento da inserire
//  head: lista in cui inserire new
//   return: void

void freePcb(pcb_t *p) {
    if (p != NULL) {
        if (list_empty(&pcbFree_h)) {      // 1 empty  0 altrimenti
            INIT_LIST_HEAD(&pcbFree_h);
        }
     list_add_tail(&p->p_list,&pcbFree_h);
    }

}


//return NULL if the pcbFree list is
//empty. Otherwise, remove an element from the pcbFree list,
//provide initial values for ALL of the PCBs fields and then
//return a pointer to the removed element. PCBs get reused, so
//it is important that no previous value persist in a PCB when it
//gets reallocated.

pcb_t *allocPcb() {
    if (list_empty(&pcbFree_h)) return NULL; // Return NULL if the pcbFree list is empty}
    else{
        struct list_head *entry = pcbFree_h.next;  //puntatore primo elemento
        list_del(entry);  
        pcb_t *new_pcb = container_of(entry, pcb_t, p_list); 

        new_pcb->p_parent = NULL;  
        INIT_LIST_HEAD(&new_pcb->p_child);  
        INIT_LIST_HEAD(&new_pcb->p_sib);    
        INIT_LIST_HEAD(&new_pcb->msg_inbox); 
        new_pcb->p_supportStruct = NULL;  
        new_pcb->p_pid = next_pid++;  
        new_pcb->p_time = 0;  
        
        return (new_pcb);
    }
}


//The queues of PCBs to be manipulated, which are called process queues, are all double, circularly
//linked lists, via the p_list field.
//To support process queues there should be the following externally visible functions:
//This method is used to initialize a variable to be head pointer to a process queue.

void mkEmptyProcQ(struct list_head *head) {
    INIT_LIST_HEAD(head);
}


//Return TRUE if the queue whose head is pointed to by head is empty. Return FALSE otherwise.

int emptyProcQ(struct list_head *head) {
    return (list_empty(head));
}


//Insert the PCB pointed by p into the process queue whose head pointer is pointed to by head.

void insertProcQ(struct list_head* head, pcb_t* p) {
    list_add_tail(&p->p_list, head);

}


//return a pointer to the first PCB from the process queue whose head is
//pointed to by head. Do not remove this PCB from the
//process queue. Return NULL if the process queue is empty.

pcb_t* headProcQ(struct list_head* head) {
    if(emptyProcQ(head)) return NULL;
    else return (container_of(head->next, pcb_t, p_list));

}

//Remove the first (i.e. head) element from the process queue whose head pointer is pointed to
//by head. Return NULL if the process queue was initially empty; otherwise return the pointer
//to the removed element.

pcb_t* removeProcQ(struct list_head* head) {
    if(emptyProcQ(head)) return NULL;
    else{
        struct list_head *entry = head->next;  // Puntatore al primo elemento
        list_del(entry); 
        return container_of(entry, pcb_t, p_list);  //return: puntatore alla struct che contiene il list_head puntato da ptr
    }
}

//Remove the PCB pointed to by p from the process queue whose head pointer is pointed to by
//head. If the desired entry is not in the indicated queue (an error condition), return NULL;
//otherwise, return p. Note that p can point to any element of the process queue.

pcb_t* outProcQ(struct list_head* head, pcb_t* p) {
    struct list_head *pos;
    pcb_t *pcb_found=NULL;

    //costruisce un ciclo for per iterare su ogni elemento della lista (pos) che ha inizio in head.
    list_for_each(pos,head){
        pcb_t *tmp=container_of(pos, pcb_t, p_list);
        if(tmp==p){
            pcb_found=tmp;
            break;
        }
    }
    if(pcb_found!=NULL) list_del(&pcb_found->p_list);
    return (pcb_found);


}


//return TRUE if the PCB
//pointed to by p has no children. Return FALSE otherwise.

int emptyChild(pcb_t *p) {
    return (list_empty(&p->p_child));
    
}

//Make the PCB pointed to by p a child of the PCB pointed to by prnt.

void insertChild(pcb_t *prnt, pcb_t *p) {
    if((prnt!=NULL)&&(p!=NULL)){
        //if(emptyChild(prnt)) INIT_LIST_HEAD(&prnt->p_child);
        list_add_tail(&p->p_sib, &prnt->p_child);
        p->p_parent=prnt;
    }
}

//Make the first child of the PCB pointed to by p no longer a child of p. Return NULL if initially
//there were no children of p. Otherwise, return a pointer to this removed first child PCB.

pcb_t* removeChild(pcb_t *p) {
    if(emptyChild(p)) return (NULL);
    else{
        struct list_head *entry = p->p_child.next;                  
        list_del(entry);  
        pcb_t *removed_childe = container_of(entry, pcb_t, p_sib);

        removed_childe->p_parent=NULL;
        return (removed_childe);
        
    }
}

//Make the PCB pointed to by p no longer the child of its parent. If the PCB pointed to by p has
//no parent, return NULL; otherwise, return p. Note that the element pointed to by p could be
//in an arbitrary position (i.e. not be the first child of its parent).

pcb_t* outChild(pcb_t *p) {
    if((p==NULL)||((p->p_parent)==NULL)) return (NULL);
    list_del(&p->p_sib);
    p->p_parent=NULL;
    return p;

}

*/