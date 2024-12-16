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
    { //klog_print("ENTRA NEL CICLO!");
    list_for_each_entry(entry, &semd_h, s_link)
    {   // controllo che esista un semaforo che abbia come indirizzo semAdd, sennò aggiungo.
        /* controlliamo nella lista attiva, se non c'è è sicuro che sia nei non attivi. ? fare contrario*/
     //klog_print("RIESCO AD ITERARE");
        if (entry->s_key == semAdd)
        {
            // klog_print("TROVA");
            list_add_tail(&p->p_list, &entry->s_procq);
            return FALSE; // finito parte 1, ha inserito il processo nella lista.
        }
    }
    }
    // NON TROVATO
    //klog_print("NON TROVATO, inizio a fare lista!");
    if (list_empty(&semdFree_h))
        return TRUE;
    
    struct list_head *semaforo = list_next(&semdFree_h);
    list_del(semaforo);
    semd_t *semDaInserire = container_of(semaforo, semd_t, s_link);
    semDaInserire->s_key = semAdd;
    INIT_LIST_HEAD(&semDaInserire->s_procq); // qua dice di fare un makeEmptyProcQ
    list_add_tail(&p->p_list, &semDaInserire->s_procq);
    list_add_tail(semaforo, &semd_h);
    return FALSE;
    // list_add_tail(); aggiungere alla coda.
}
/**/

pcb_t *removeBlocked(int *semAdd)
{
    int find = FALSE;
    semd_t *entry;
    semd_t *sem_saved; 
    klog_print("Entro nella funzione");
    list_for_each_entry(entry, &semd_h, s_link){
        klog_print("ENTRO");
        if(entry->s_key == semAdd){
            find = TRUE; 
            sem_saved = entry; 
        }
    }
    if(!find) return NULL; 
    else{
        klog_print("Entro nell'else");
        struct list_head *first_pcb = list_next(&sem_saved->s_procq);
        struct pcb_t *pcb_rem = container_of(first_pcb, pcb_t, p_list);
        //Ho salvato in pcb_rem il primo pcb del semaforo trovato
        list_del(first_pcb); 
        if(emptyProcQ(&sem_saved->s_procq)){
           // list_del(first_pcb);
            list_add_tail(&sem_saved->s_procq, &semdFree_h);
        }
        return pcb_rem;
    }
}

pcb_t *outBlockedPid(int pid)
{
}

pcb_t *outBlocked(pcb_t *p)
{
}

pcb_t *headBlocked(int *semAdd)
{
}
