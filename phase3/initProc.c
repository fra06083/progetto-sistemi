 /*
 This module implements test N.B. usato come Istantiator process 
 - per inizializzazione vedi da sez 9 in poi 
 */

#include "../headers/const.h"
#include "../headers/types.h"

#define DBit 2 // Dirty bit
#define VBit 1 // Valid bit
#define GBit 0 // Global bit

//Dichiarazioni (globali) delle variabili di fase 3 (qua)
//N.B. Nella documentazione in alcuni casi possiamo scegliere di dichiararle localmente nei file! 

support_t sup_struct[8]; //8 U-proc, per ogni  struttura di supporto 
swap_t swap_pool[POOLSIZE]; // nel documento dice uprocmax * 2
int swap_mutex; // semaforo mutua esclusione pool
int asidAcquired; // asid che prende la mutua esclusione
#define STACKVPN (ENTRYHI_VPN_MASK >> VPNSHIFT)
unsigned int getPageIndex(unsigned int entry_hi)
{
    unsigned int vpn = ENTRYHI_GET_VPN(entry_hi);
    if (vpn == STACKVPN) { 
        return USERPGTBLSIZE-1; // qui controlliamo se è lo stack, dobbiamo ritornare 31 se sì
    } else { 
        return vpn;
    }
}

void initPageTable(support_t *sup, int asid) {
    
    for (int i = 0; i < USERPGTBLSIZE; i++) {
        unsigned int vpn;
        vpn = (i<31) ? 0x80000 + i : 0xBFFFF;

        unsigned int entryHI = (vpn << VPNSHIFT) | (asid & 0xFFF); // Calcola l'entry HI con il VPN e l'ASID
        unsigned int entryLO = (1 << DBit)|(0 << GBit) | (0 << VBit); // D=1, G=0, V=0

        sup->sup_privatePgTbl[i].pte_entryHI = entryHI; // Inizializza l'entry HI
        sup->sup_privatePgTbl[i].pte_entryLO = entryLO; // Inizializza l'entry LO
    }
}

// Questa funzione ci serve per testare i vari componenti della fase 3 
void p3test(){
    // initialize the swap pool as written in documentation: to access P on sem_mutex then V
    for (int i = 0; i < POOLSIZE; i++) {
        swap_pool[i].sw_asid = -1;
        swap_pool[i].sw_pageNo = 0;
        swap_pool[i].sw_pte    = NULL
        
    }
    swap_mutex = 1;
    for(int i = 0; i < UPROCMAX; i++) {
        sup_struct[i].sup_asid = i+1 ; // Setta ASID [1...8] per ogni processo utente
        initPageTable(&sup_struct[i], i + 1); // Inizializza la page table per ogni U-proc 

        //qui si aggiungerà poi il resto
    }
    

//Per il momento setta qui tutte le entry della proc page table per ogni U-proc (supp_struct), 
//segui documentazione VPN field, ASID field, ecc... 
//sup_asid, sup_exceptContext[2], and sup_privatePgTbl[32] [Section 2.1] require
//initialization prior to request the CreateProcess service

}





