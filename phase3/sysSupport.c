#include "./headers/sysSupport.h"
/* TODO: 
- General exception handler [Section 6].
- SYSCALL exception handler [Section 7].
- Program Trap exception handler [Section 8].
*/
// Punto 6/7
void SYS2() {
   SYSCALL(TERMINATE, 0, 0, 0); // Termina il processo corrente (dopo che il valore è stato messo in reg_a0, esegui la SYSCALL)
    //E' un wrapper, non devo fare altro
}

void SYS3(state_t *stato){
   char *VAddr_first_char = (char *) stato->reg_a1; 
      int str_len = stato->reg_a2;
      //Controllo che l'indirizzo virtuale del primo char sia entro il logical U-Proc Address Space, non ecceda la lunghezza del  e che la lunghezza della stringa sia accettabile
      //In teoria nel logical U-Proc Address Space è limitato da sotto da UPROCSTARTADDR e sopra da USERSTACKTOP 
      if((str_len < 0 || str_len > 128) || VAddr_first_char < (char *) UPROCSTARTADDR || VAddr_first_char >= (char *) USERSTACKTOP) {
          SYS2();
          return; 
      }
      int retStatus = SYSCALL(WRITEPRINTER, (unsigned int) VAddr_first_char, str_len, 0);
      //Devo vedere che tipo di errore mi ritorna, se è diverso da 1 allora c'è stato un errore 
      if(retStatus != 1) {
          stato->reg_a0 = -retStatus;
      }
      stato->pc_epc += 4; // incrementa il program counter
      LDST(stato); // ripristina lo stato del processo corrente 
}






void generalExceptionHandler(){
    support_t *sup = (support_t *) SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    // determiniamo la causa
    state_t* state = &(sup->sup_exceptState[GENERALEXCEPT]);

    //Decodifica l'eccezione: se NON è SYSCALL -> Program Trap 
    unsigned int exccode = (state->cause & GETEXECCODE) >> CAUSESHIFT;  // mask/shift definiti in const.h
    if (exccode != SYSEXCEPTION) {
        supportTrapHandler(sup);   // Program Trap handler 
        return;                    // non proseguire nel dispatch SYSCALL
    }

    //incremento così passo all'istruzione dopo la syscall (cap7)
    state->pc_epc += 4; //incremento di 4 il program counter che ha causato l'eccezione

    switch (state->reg_a0){
        case TERMINATE:   // SYS2
            SYS2();
            break;

        case WRITEPRINTER: // SYS3
            SYS3(state);
            break;

        case WRITETERMINAL: // SYS4
            // TODO: WRITE TO TERMINAL
            break;

        case READTERMINAL:  // SYS5
            // TODO: READ FROM TERMINAL
            break;

        default:
            supportTrapHandler(sup);
            break;
    }
}



void supportTrapHandler(support_t *sup_ptr){         
  extern int swap_mutex;
  extern int asidAcquired;
  
  if(asidAcquired== sup_ptr->sup_asid) {
    SYSCALL(VERHOGEN, (int)&swap_mutex, 0, 0); //NSYS4
    asidAcquired = -1;
  }
  SYSCALL(TERMINATE, 0, 0, 0); // Termina il processo corrente 
}