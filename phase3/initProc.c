#include "./headers/initProc.h"
 /*
 This module implements test N.B. usato come Istantiator process 
 - per inizializzazione vedi da sez 9 in poi 
 */

extern void klog_print(const char *str);
extern void klog_print_dec(unsigned int num);

//DICHIARAZIONI VARIABILI GLOBALI - N.B. Nella documentazione in alcuni casi possiamo scegliere di dichiararle localmente nei file

int masterSem; // punto 10 -  zero-initialized
state_t procStates[UPROCMAX];
support_t sup_struct[UPROCMAX]; //8 U-proc, per ogni  struttura di supporto 

swap_t swap_pool[POOLSIZE]; // punto 4.1 -> 2*UPROCMAX -> insieme di frame dedicati al paging
int swap_mutex; // semaforo mutua esclusione pool

int asidAcquired; // asid che prende la mutua esclusione

int supportSem[NSUPPSEM]; // Dal punto 9 ci servono dei semafori supporto dei device
int supportSemAsid[UPROCMAX]; // array che dice quale asid ha acquisito il device i-esimo, -1 se nessuno


//X MUTUAL EXCLUSION I/O
void acquireDevice(int asid, int devIndex) {
    int* sem = &supportSem[devIndex];
    SYSCALL(PASSEREN, (int)sem, 0, 0); //P(sem) - NSYS3
    supportSemAsid[asid-1] = devIndex;           
}
void releaseDevice(int asid, int deviceIndex) {
    int* sem = &supportSem[deviceIndex];
    supportSemAsid[asid-1] = -1;              
    SYSCALL(VERHOGEN, (int)sem, 0, 0);
    
}

//X MUTUAL EXCLUSION SWAP POOL
void acquire_mutexTable(int asid){
    SYSCALL(PASSEREN, (int)&swap_mutex, 0, 0);
    asidAcquired = asid;
}
void release_mutexTable(){
    asidAcquired = -1;
    SYSCALL(VERHOGEN, (int)&swap_mutex, 0, 0);
}


// X  ottenere index della pagina a partire da un entry_hi
unsigned int getPageIndex(unsigned int entry_hi)
{
    unsigned int vpn = ENTRYHI_GET_VPN(entry_hi);
    if (vpn == STACKVPN) { 
        return USERPGTBLSIZE-1; // qui controlliamo se è lo stack, dobbiamo ritornare 31 se sì
    } else { 
        return vpn; // text + data
    }
}


// X testare i vari componenti della fase 3 
void p3test(){
    //INIZIALIZZAZIONE SWAP POOL punto 4.1
    // to access P on sem_mutex then V
    for (int i = 0; i < POOLSIZE; i++) {
        swap_pool[i].sw_asid = -1; //frame libero
        swap_pool[i].sw_pageNo = 0;
        swap_pool[i].sw_pte    = NULL;
    }
    swap_mutex = 1;

    // FIX: i semafori di supporto si inizializzano una sola volta fuori dal ciclo
    for (int i = 0; i < NSUPPSEM; i++) supportSem[i] = 1; 
    for (int i = 0; i < UPROCMAX; i++) supportSemAsid[i] = -1; // -1 significa che non ha acquisito nessun device
    masterSem = 0; 
    asidAcquired = -1; //nessuno ha acquisito il mutex

    // INIZIALIZZAZIONE PROCESSI
    for (int i = 0; i < UPROCMAX; i++) {
        int ASID = i+1;

        // Iniziazione stati
        procStates[i].pc_epc = UPROCSTARTADDR;
        procStates[i].reg_sp = USERSTACKTOP;
        procStates[i].status = MSTATUS_MPIE_MASK; 
        procStates[i].mie = MIE_ALL; // abilita interrupts
        procStates[i].entry_hi = ASID << ASIDSHIFT; // FIX: shift dell’ASID

        // struct di supporto 0....7
        sup_struct[i].sup_asid = ASID;

        // inizializzazione per TLB
        sup_struct[i].sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr)uTLB_ExceptionHandler;
        sup_struct[i].sup_exceptContext[PGFAULTEXCEPT].status = MSTATUS_MPP_M;
        sup_struct[i].sup_exceptContext[PGFAULTEXCEPT].stackPtr = (memaddr)&(sup_struct[i].sup_stackTLB[499]); // gestiamo come stack dal fondo
        
        // inizializzazione per General Exception
        sup_struct[i].sup_exceptContext[GENERALEXCEPT].pc = (memaddr)generalExceptionHandler;
        sup_struct[i].sup_exceptContext[GENERALEXCEPT].status = MSTATUS_MPP_M;
        sup_struct[i].sup_exceptContext[GENERALEXCEPT].stackPtr = (memaddr)&(sup_struct[i].sup_stackGen[499]);

        //Per il momento setta qui tutte le entry della proc page table per ogni U-proc (supp_struct), 
        //segui documentazione VPN field, ASID field, ecc... 
        //sup_asid, sup_exceptContext[2], and sup_privatePgTbl[32] [Section 2.1] require
        //initialization prior to request the CreateProcess service
        for (int j = 0; j < USERPGTBLSIZE; j++) {
            unsigned int vpn;
            vpn = (j != USERPGTBLSIZE-1) ? (KUSEG | (j << VPNSHIFT)) : 0xBFFFF000; // FIX: uso j, non i
            // se è uguale calcolo il suo indirizzo, sennò valore finale 0xBFFFF000
            unsigned int entryHI = vpn | (ASID << ASIDSHIFT); // FIX: shift dell’ASID
            unsigned int entryLO = DIRTYON; 

            sup_struct[i].sup_privatePgTbl[j].pte_entryHI = entryHI; // Inizializza l'entry HI
            sup_struct[i].sup_privatePgTbl[j].pte_entryLO = entryLO; // Inizializza l'entry LO
        }
        SYSCALL(CREATEPROCESS, (int)&(procStates[i]), 0, (int)&(sup_struct[i])); // FIX: uso sup_struct
    }

    //Attesa terminazione processi
    for (int i = 0; i < UPROCMAX; i++) { // P così aspetta che termini
        //print("Waiting for process to terminate...\n");
        SYSCALL(PASSEREN, (int)&masterSem, 0, 0);
    }

    klog_print("Terminiamo il processo principale ora.\n");
    SYSCALL(TERMPROCESS, 0, 0, 0);
}
