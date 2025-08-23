#include "./headers/initProc.h"
 /*
 This module implements test N.B. usato come Istantiator process 
 - per inizializzazione vedi da sez 9 in poi 
 */

extern void klog_print(const char *str);
extern void klog_print_dec(unsigned int num);
//Dichiarazioni (globali) delle variabili di fase 3 (qua)
//N.B. Nella documentazione in alcuni casi possiamo scegliere di dichiararle localmente nei file! 
int masterSem; // alla fine dice di gestirlo così
state_t procStates[UPROCMAX] = {0};
support_t sup_struct[UPROCMAX] = {0}; //8 U-proc, per ogni  struttura di supporto 
swap_t swap_pool[POOLSIZE]; // nel documento dice uprocmax * 2
int swap_mutex; // semaforo mutua esclusione pool
int asidAcquired; // asid che prende la mutua esclusione
int supportSem[NSUPPSEM]; // Dal punto 9 ci servono dei semafori supporto dei device
int supportSemAsid[UPROCMAX];

void acquireDevice(int asid, int devIndex) {
    klog_print("Acquire Device \n");
    int* sem = &supportSem[devIndex];
    SYSCALL(PASSEREN, (int)sem, 0, 0);
    supportSemAsid[asid-1] = devIndex;
}
void releaseDevice(int asid, int deviceIndex) {
    int* sem = &supportSem[deviceIndex];
    supportSemAsid[asid-1] = -1;
    SYSCALL(VERHOGEN, (int)sem, 0, 0);
}

void acquireSwapPoolTable(int asid) {
    klog_print("Acquire SWAP \n");
    SYSCALL(PASSEREN, (int)&swap_mutex, 0, 0);
    asidAcquired = asid;
}
void releaseSwapPoolTable() {
    klog_print("RELEASE SWAP \n");
    asidAcquired = -1;
    SYSCALL(VERHOGEN, (int)&swap_mutex, 0, 0);
}
unsigned int getPageIndex(unsigned int entry_hi)
{
    unsigned int vpn = ENTRYHI_GET_VPN(entry_hi);
    if (vpn == STACKVPN) { 
        return USERPGTBLSIZE-1; // qui controlliamo se è lo stack, dobbiamo ritornare 31 se sì
    } else { 
        return vpn;
    }
}


// Questa funzione ci serve per testare i vari componenti della fase 3 
void p3test(){
    // initialize the swap pool as written in documentation: to access P on sem_mutex then V
//    klog_print("Initializing swap pool and support structures...\n");
    for (int i = 0; i < POOLSIZE; i++) {
        swap_pool[i].sw_asid = -1;
        swap_pool[i].sw_pageNo = 0;
        swap_pool[i].sw_pte    = NULL;
    }
    swap_mutex = 1;
    masterSem = 0;
    asidAcquired = -1;

    // FIX: i semafori di supporto si inizializzano una sola volta fuori dal ciclo
    for (int i = 0; i < NSUPPSEM; i++) supportSem[i] = 1; 
    for (int i = 0; i < UPROCMAX; i++) supportSemAsid[i] = -1; // -1 significa che non ha acquisito nessun device
    // inizializzazione processi:
    for (int i = 0; i < UPROCMAX; i++) {
//        klog_print("Initializing processes\n");
        int ASID = i+1;

        // Iniziazione stati
        procStates[i].pc_epc = UPROCSTARTADDR;
        procStates[i].reg_sp = USERSTACKTOP;
        procStates[i].status = MSTATUS_MPIE_MASK;
        procStates[i].mie = MIE_ALL;
        procStates[i].entry_hi = ASID << ASIDSHIFT; // FIX: shift dell’ASID

        // struct di supporto 0....7
        sup_struct[i].sup_asid = ASID;

        /* CONTEXT TLB EXC. HANDLER */
        context_t context_TLB;
        context_TLB.pc = (memaddr)
        uTLB_ExceptionHandler;
        context_TLB.status = MSTATUS_MPP_M;
        context_TLB.stackPtr = (memaddr)&(sup_struct[i].sup_stackTLB[499]); // gestiamo come stack dal fondo

        /* CONTEXT GENERAL EXC. HANDLER*/
        context_t context_GE;
        context_GE.pc = (memaddr) generalExceptionHandler;
        context_GE.status = MSTATUS_MPP_M;
        context_GE.stackPtr = (memaddr)&(sup_struct[i].sup_stackGen[499]); // da sopra, lo gestiamo come stack

        // assegniamo i context alle strutture di supporto
        sup_struct[i].sup_exceptContext[PGFAULTEXCEPT] = context_TLB; // TLB exception
        sup_struct[i].sup_exceptContext[GENERALEXCEPT] = context_GE; // general exception
//        klog_print("ok settato\n");

        //Per il momento setta qui tutte le entry della proc page table per ogni U-proc (supp_struct), 
        //segui documentazione VPN field, ASID field, ecc... 
        //sup_asid, sup_exceptContext[2], and sup_privatePgTbl[32] [Section 2.1] require
        //initialization prior to request the CreateProcess service
        for (int j = 0; j < USERPGTBLSIZE; j++) {
            unsigned int vpn;
            vpn = (j != USERPGTBLSIZE-1) ? KUSEG | (j << VPNSHIFT) : 0xBFFFF000; // FIX: uso j, non i
            // se è uguale calcolo il suo indirizzo, sennò valore finale 0xBFFFF000
            unsigned int entryHI = vpn | (ASID << ASIDSHIFT); // FIX: shift dell’ASID
            unsigned int entryLO = DIRTYON; 

            sup_struct[i].sup_privatePgTbl[j].pte_entryHI = entryHI; // Inizializza l'entry HI
            sup_struct[i].sup_privatePgTbl[j].pte_entryLO = entryLO; // Inizializza l'entry LO
        }
//        klog_print("creazione processo\n");
        SYSCALL(CREATEPROCESS, (int)&(procStates[i]), 0, (int)&(sup_struct[i])); // FIX: uso sup_struct
    }

    for (int i = 0; i < UPROCMAX; i++) { // P così aspetta che termini
        print("Waiting for process to terminate...\n");

        SYSCALL(PASSEREN, (int)&masterSem, 0, 0);
    }

    print("Dovrebbe essere finito qui, terminiamo il processo principale\n");
    SYSCALL(TERMPROCESS, 0, 0, 0);
}
