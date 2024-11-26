#include "./headers/pcb.h"

static struct list_head pcbFree_h; // sentinella della lista dei processi liberi o non utilizzati, NON NE FA PARTE.
static pcb_t pcbFree_table[MAXPROC]; // tabella processi
static int next_pid = 1; // prossimo id del processo

/*



*/
void initPcbs() {
 INIT_LIST_HEAD(&pcbFree_h);
for (int i = 0; i<MAXPROC; i++){
}
}

void freePcb(pcb_t* p) { // inserire l'elemento p nella lista pcbfree ()
   // pcb_PTR lista = 
   // for (int i = 0)
}

pcb_t* allocPcb() {
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

int emptyChild(pcb_t* p) {
}

void insertChild(pcb_t* prnt, pcb_t* p) {
}

pcb_t* removeChild(pcb_t* p) {
}

pcb_t* outChild(pcb_t* p) {
}