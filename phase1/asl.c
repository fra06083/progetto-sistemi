#include "./headers/asl.h"

static semd_t semd_table[MAXPROC];
static struct list_head semdFree_h; // semafori non utilizzati
static struct list_head semd_h;     // semafori attivi ASL LIST
#include "./headers/pcb.h"
void initASL()
{

    INIT_LIST_HEAD(&semdFree_h); // inizializzo la testa
    INIT_LIST_HEAD(&semd_h);
    for (int i = 0; i < MAXPROC; i++)
    {
        semd_t *puntatore = &semd_table[i];             // qua prendo i semafori della tabella
        list_add_tail(&puntatore->s_link, &semdFree_h); // perché non sono ancora utilizzati
    }
}
/*
 insert the
PCB pointed to by p at the tail of the process queue
associated with the semaphore whose key is semAdd and set
the semaphore address of p to semaphore with semAdd. If the
semaphore is currently not active, allocate a new descriptor
from the semdFree list, insert it in the ASL (at the
appropriate position), initialize all of the fields, and proceed as
above. If a new semaphore descriptor needs to be allocated
and the semdFree list is empty, return TRUE. In all other
cases return FALSE.
*/
int insertBlocked(int *semAdd, pcb_t *p)
{
    // S
    semd_t *entry;
    if (!list_empty(&semd_h)){
        list_for_each_entry(entry, &semd_h, s_link)
        { // controllo che esista un semaforo che abbia come indirizzo semAdd, sennò aggiungo.
            /* controlliamo nella lista attiva, se non c'è è sicuro che sia nei non attivi. */
            if (entry->s_key == semAdd)
            {
                //Semaforo trovato, lo aggiungo nella lista 
                list_add_tail(&p->p_list, &entry->s_procq);
                p->p_semAdd = semAdd;
                return FALSE; 
            }
        }
    }
    //Semaforo non trovato, se la lista dei semafori inutilizzati è vuota allora return True
    if(list_empty(&semdFree_h))
        return TRUE;
    // Altrimenti, estraggo il primo semaforo della lista dei semafori non utilizzati e metto i campi corretti.
    struct list_head *semaforo = list_next(&semdFree_h);
    semd_t *semDaInserire = container_of(semaforo, semd_t, s_link);
    semDaInserire->s_key = semAdd;
    p->p_semAdd = semAdd;
    //Rimosso dalla lista dei semafori inutilizzati il semDaInserire e inizializzo il campo s_procq con mkEmptyProcQ
    list_del(&semDaInserire->s_link);
    mkEmptyProcQ(&semDaInserire->s_procq); 
    //Aggiungo la process queue di p nella queue dei processi bloccati su quel semaforo 
    list_add_tail(&p->p_list, &semDaInserire->s_procq);
    //Aggiungo il semaforo nella lista dei semafori attivi 
    list_add_tail(&semDaInserire->s_link, &semd_h);
    p->p_semAdd = semAdd; // setto il semaforo di p
    return FALSE;
}
/*
 search the ASL for
a descriptor of this semaphore. If none is found, return NULL;
otherwise, remove the first PCB from the process queue of the
found semaphore descriptor and return a pointer to it. If the
process queue for this semaphore becomes empty, remove the
semaphore descriptor from the ASL and return it to the
semdFree list.
*/

pcb_t *removeBlocked(int *semAdd)
{
    semd_t *entry;
    list_for_each_entry(entry, &semd_h, s_link)
    {
        if (entry->s_key == semAdd)
        {
            // salvo il primo visto che removePROCQ mi cancella il primo processo
            pcb_t *first = removeProcQ(&entry->s_procq);
            if (emptyProcQ(&entry->s_procq))
            {
                list_del(&entry->s_link);
                list_add_tail(&entry->s_link, &semdFree_h);
                /*
                lo rinserisco nella semdFree_h visto che la coda è vuota non è più utilizzata
                */
            }
            return first;
        }
    }
    return NULL;
}
/*
Remove the PCB pointed to by p from the process queue associated with p’s semaphore (p->p_semAdd)
on the ASL. If PCB pointed to by p does not appear in the process queue associated with p’s
semaphore, which is an error condition, return NULL; otherwise, return p.

*/
pcb_t *outBlocked(pcb_t *p)
{
  // cerco nei semafori attivi
 semd_t *entry = NULL;
 list_for_each_entry(entry, &semd_h, s_link)
    {
        if (entry->s_key == p->p_semAdd)
        {
           pcb_t* processi = outProcQ(&entry->s_procq, p); // rimuovo la process queue da p per poi restituirlo.
           if (emptyProcQ(&entry->s_procq))
            {
                list_del(&entry->s_link);
                list_add_tail(&entry->s_link, &semdFree_h);
                /*
                lo rinserisco nella semdFree_h visto che la coda è vuota non è più utilizzata
                */
            }
           return processi;
        }
    }
 return NULL; // non trovato quindi è error condition
}
/* FUNZIONE AGGIUNTA IN FASE 2:
In sostanza è una funzione che cerca un processo in blocked queue
e se lo trova lo rimuove dalla blocked queue (se remove è >0) e lo restituisce.
*/
pcb_t *findBlockedPid(int pid)
{
    semd_t *entry = NULL;
    list_for_each_entry(entry, &semd_h, s_link)
    {
        pcb_t *processo = NULL;
        list_for_each_entry(processo, &entry->s_procq, p_list)
        {
            if (processo->p_pid == pid)
            {
                return processo;
            }
        }
    }
    return NULL; // non trovato quindi è error condition
}
/*
Return a pointer to the PCB that is at the head of the process queue associated with the
semaphore semAdd. Return NULL if semAdd is not found on the ASL or if the process queue
associated with semAdd is empty.
*/
pcb_t *headBlocked(int *semAdd)
{
    semd_t *entry;
 list_for_each_entry(entry, &semd_h, s_link)
    {
        if (entry->s_key == semAdd)
        {
            // trovato
          return headProcQ(&entry->s_procq);
        }
    }
    return NULL; // semAdd is not found.
}

/* FUNZIONE AGGIUNTA IN FASE 2, trova il semaforo e lo restituisce */
semd_t *findSemaphore(int *semAdd) {
    semd_t *entry = NULL;
    list_for_each_entry(entry, &semd_h, s_link) {
        if (entry->s_key == semAdd) {
            return entry; // trovato
        }
    }
    return NULL; // non trovato
}