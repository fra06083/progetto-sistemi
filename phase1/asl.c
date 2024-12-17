#include "./headers/asl.h"

static semd_t semd_table[MAXPROC];
static struct list_head semdFree_h; // semafori non utilizzati
static struct list_head semd_h;     // semafori attivi ASL LIST
#include "../klog.c"
#include "./headers/pcb.h"
/*

liste = semdFree_h.next
liste
*/
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
int insertBlocked(int *semAdd, pcb_t *p)
{
    // SE VUOTO INIZIALIZZIAMO, SE NON VUOTO AGGIUNGIAMO IN CODA.
    semd_t *entry;
    if (!list_empty(&semd_h))
    { // klog_print("ENTRA NEL CICLO!");
        list_for_each_entry(entry, &semd_h, s_link)
        { // controllo che esista un semaforo che abbia come indirizzo semAdd, sennò aggiungo.
            /* controlliamo nella lista attiva, se non c'è è sicuro che sia nei non attivi. ? fare contrario*/
            // klog_print("RIESCO AD ITERARE");
            if (entry->s_key == semAdd)
            {
                // klog_print("TROVA");
                list_add_tail(&p->p_list, &entry->s_procq);
                p->p_semAdd = semAdd;
                return FALSE; // finito parte 1, ha inserito il processo nella lista.
            }
        }
    }
    // NON TROVATO
    // klog_print("NON TROVATO, inizio a fare lista!");
    if (list_empty(&semdFree_h))
        return TRUE;

    struct list_head *semaforo = list_next(&semdFree_h);
    // list_del(semaforo);
    semd_t *semDaInserire = container_of(semaforo, semd_t, s_link);
    semDaInserire->s_key = semAdd;
    p->p_semAdd = semAdd;
    list_del(&semDaInserire->s_link);
    mkEmptyProcQ(&semDaInserire->s_procq); // qua dice di fare un makeEmptyProcQ
    list_add_tail(&p->p_list, &semDaInserire->s_procq);
    list_add_tail(&semDaInserire->s_link, &semd_h);
    return FALSE;
    // list_add_tail(); aggiungere alla coda.
}
/**/

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
                list_add_tail(&entry->s_link, &semdFree_h); // no devo spostare il semaforo
            }
            return first;
        }
    }
    return NULL;
}

pcb_t *outBlockedPid(int pid)
{
}
/*

Remove the PCB pointed to by p from the process queue associated with p’s semaphore (p->p_semAdd)
on the ASL. If PCB pointed to by p does not appear in the process queue associated with p’s
semaphore, which is an error condition, return NULL; otherwise, return p.
simile outProcQ?
*/
pcb_t *outBlocked(pcb_t *p)
{
  // cerco nei semafori attivi
 semd_t *entry = NULL;
 list_for_each_entry(entry, &semd_h, s_link)
    {
        klog_print_dec(entry->s_key);
        if (entry->s_key == p->p_semAdd)
        {
            // TROVO
            klog_print("ENTRO");
           return outProcQ(&entry->s_procq, p); // nella process queue, procedo su p.
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
