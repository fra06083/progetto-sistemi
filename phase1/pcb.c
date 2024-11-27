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
