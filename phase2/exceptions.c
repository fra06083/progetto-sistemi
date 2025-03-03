void uTLB_RefillHandler() {
    // Ottiene l'ID del processore corrente
    int prid = getPRID();
   
    // Imposta il registro ENTRYHI a 0x80000000
    setENTRYHI(0x80000000);
    
    // Imposta il registro ENTRYLO a 0x00000000
    setENTRYLO(0x00000000);
    
    // Scrive l'entrata nel TLB (Translation Lookaside Buffer)
    TLBWR();
    
    // Carica e ripristina lo stato dell'eccezione per il processore corrente
    LDST(GET_EXCEPTION_STATE_PTR(prid));
}