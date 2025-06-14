 /*
 This module implements test N.B. usato come Istantiator process 
 - per inizializzazione vedi da sez 9 in poi 
 */

#include "../headers/const.h"
#include "../headers/types.h"

//Dichiarazioni (globali) delle variabili di fase 3 (qua)
//N.B. Nella documentazione in alcuni casi possiamo scegliere di dichiararle localmente nei file! 

support_t supp_struct[8]; //8 U - proc, per ognuno struttura di supporto 


//Sta funzione non ho ancora ben capito dove e quando chiamarla, ma di base deve inizializzare le strutture che dichiariamo in questo file 
void test(){


//Per il momento setta qui tutte le entry della proc page table per ogni U - proc (supp_struct), segui documentazione VPN field, ASID field, ecc... 
//sup_asid, sup_exceptContext[2], and sup_privatePgTbl[32] [Section 2.1] require
//initialization prior to request the CreateProcess service

}





