#include "./headers/sysSupport.h"
/* TODO: 
- General exception handler [Section 6].
- SYSCALL exception handler [Section 7].
- Program Trap exception handler [Section 8].
*/
// Punto 6/7
extern int masterSem;
void TerminateSYS(int asidTerminate) {
    if (asidAcquired != asidTerminate) {
        acquireSwapPoolTable(asidTerminate); // va fatto in mutuaesclusione!!
    }
    releaseSwapPoolTable();

    int deviceIndex = supportSemAsid[asidTerminate-1]; // release device if the uprocs was holding it
    if (deviceIndex != -1) {
        releaseDevice(asidTerminate, deviceIndex);
    }

    SYSCALL(VERHOGEN, (int)&masterSem, 0, 0); // dobbiamo fare una V al masterSem quando terminiamo un processo
    SYSCALL(TERMINATE, 0, 0, 0); // Termina il processo corrente (dopo che il valore è stato messo in reg_a0, esegui la SYSCALL)
    //E' un wrapper, non devo fare altro
}
// print di p2test, scrive nel terminale term, msg di lunghezza lenMsg
int printTerminal(char* msg, int lenMsg, int term) {
    termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, term);
    unsigned int status;
    unsigned int value;
    int charSent = 0;
    while (charSent < lenMsg) { // loop di un carattere alla volta; come visto in print di p2test
        value = PRINTCHR | (((unsigned int)*msg) << 8);
        status = SYSCALL(DOIO, (int)&devReg->transm_command, (int)value, 0);
        if ((status & 0xFF) != CHARRECV) {
            return -status;
        }
        msg++;
        charSent++;
    }
    return charSent;
}
int printPrinter(char* msg, int length, int devNo) {
    dtpreg_t* devReg = (dtpreg_t*)DEV_REG_ADDR(IL_FLASH, devNo);
    unsigned int status;
    unsigned int value;
    int charSent = 0;
    while (charSent < length) { // loop di un carattere alla volta; come visto in print di p2test
        value = PRINTCHR | (((unsigned int)*msg) << 8);
        status = SYSCALL(DOIO, (int)&devReg->command, (int)value, 0);
        if ((status & 0xFF) != CHARRECV) {
            return -status;
        }
        msg++;
        charSent++;
    }
    return charSent;
}

void writeDevice(state_t *stato, int asid, int type){
//    termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, type);
    char* vAddr = (char*)stato->reg_a1;
    int str_len = stato->reg_a2;
    //Controllo che l'indirizzo virtuale del primo char sia entro il logical U-Proc Address Space, non ecceda la lunghezza del  e che la lunghezza della stringa sia accettabile
    //In teoria nel logical U-Proc Address Space è limitato da sotto da UPROCSTARTADDR e sopra da USERSTACKTOP 
    if((str_len < 0 || str_len > MAXSTRLENG) || vAddr < (char *) UPROCSTARTADDR || vAddr >= (char *) USERSTACKTOP) {
        TerminateSYS(asidAcquired);
        return; 
    }
    int status;
    if (type == WRITETERMINAL) {
        memaddr* indirizzo_comando = (memaddr*) stato->reg_a1;
        int deviceIndex = findDevice(indirizzo_comando);
        acquireDevice(asid, deviceIndex);
        status = printTerminal(vAddr, str_len, deviceIndex);
        releaseDevice(asid, deviceIndex);
    } else if (type == WRITEPRINTER){
        memaddr* indirizzo_comando = (memaddr*) stato->reg_a1;
        int deviceIndex = findDevice(indirizzo_comando);
        acquireDevice(asid, deviceIndex);
        status = printPrinter(vAddr, str_len, deviceIndex);
        releaseDevice(asid, deviceIndex);
    }
    //Se l'operazione ha avuto successo, in a0 c'è il numero di caratteri trasmessi 
    stato->reg_a0 = status;
    stato->pc_epc += 4; // incrementa il program counter
    LDST(stato); // ripristina lo stato del processo corrente (va messo in teoria anche se non scritto perchè il processo viene sospeso)
}

// riceve da terminale e salva su address
int inputFromTerminal(char* addr, int term) {
    termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, term);
    int str_len = 0;
    while (1) { // reads 1 character at a time until a newline occurs
        int status = SYSCALL(DOIO, (int)&devReg->recv_command, RECEIVECHAR, 0);
        if ((status & 0xFF) != CHARRECV) {
            return -status;
        }
        char character = (char)(status >> 8); // prende il carattere, shiftato di 8 || guardare il print per capire come funziona e lo solva in addr
        *addr = character;
        addr++;
        str_len++;
        if (character == '\n') break;
    }
    return str_len;
}
void readTerminal(state_t* stato){
    char *vAddr = (char *) stato->reg_a1; 
    if(Vaddr_start_buff < (char *) UPROCSTARTADDR || Vaddr_start_buff >= (char *) USERSTACKTOP) {
        TerminateSYS(asidAcquired);
        return; 
    }
    int devNo = asid-1;
    int deviceIndex = findDevice((memaddr*) stato->reg_a1);
    acquireDevice(asid, deviceIndex);
    int status = inputTerminal(vAddr, devNo);
    releaseDevice(asid, deviceIndex);
    stato->reg_a0 = status;
   // if(status != 5){
   //     stato->reg_a0 = -retStatus; 
   // }
    //Qua dice che in a0 deve esserci il numero dei caratteri della stringa, qua come si ricava? 
    stato->pc_epc += 4; 
    LDST(stato);
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
    int asid = sup->sup_asid;
    switch (state->reg_a0){
        case TERMINATE:   // SYS2
            TerminateSYS(asid);
            break;

        case WRITEPRINTER: // SYS3
            writeDevice(state, asid, WRITEPRINTER);
            break;

        case WRITETERMINAL: // SYS4
            writeDevice(state, asid, WRITETERMINAL);
            break;

        case READTERMINAL:  // SYS5
            readTerminal(state); 
            break;

        default:
            supportTrapHandler(sup);
            break;
    }
}



void supportTrapHandler(support_t *sup_ptr){         
  TerminateSYS(sup_ptr->sup_asid);
}
