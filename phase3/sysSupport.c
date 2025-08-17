#include "./headers/sysSupport.h"
/* TODO: 
- General exception handler [Section 6].
- SYSCALL exception handler [Section 7].
- Program Trap exception handler [Section 8].
*/
// Punto 6/7
void generalExceptionHandler(){
 support_t *supp = (support_t *) SYSCALL(GETSUPPORTPTR, 0, 0, 0);
 // determiniamo la causa
 state_t* state = &(supp->sup_exceptState[GENERALEXCEPT]);
 switch (state->reg_a0){
  case TERMINATE:
    // termina
  break;
  case WRITEPRINTER:
    // writeprinter
  break;
  case WRITETERMINAL:
  // WRITE TO TERMINAL
  break;
  case READTERMINAL:
  break;
  
  default:
  // program trap handler;
  break;
 }
}
// il vpn è nei 31..12 bit di entry_hi, lo utilizzaimo come index della page table contenuta 
// in p_supportStruct

void supportTrapHandler(support_t sup_ptr){         
   //Section 8 ... 


}