#include "./headers/sysSupport.h"
/* TODO: 
- General exception handler [Section 6].
- SYSCALL exception handler [Section 7].
- Program Trap exception handler [Section 8].
*/
// Punto 6/7
extern void klog_print(const char *str);
extern void klog_print_dec(unsigned int num);
extern int masterSem;
void TerminateSYS(int asidTerminate) {
    if (asidAcquired != asidTerminate) {
        acquire_mutexTable(asidTerminate);
    }
    //acquire_mutexTable(asidTerminate);
    for (int i = 0; i < POOLSIZE; i++) {
        swap_t *swap = &swap_pool[i];
        if (swap->sw_asid == asidTerminate) {
            swap->sw_asid = -1;
        }
    } 
    release_mutexTable();         //AGGIUNTO IF: non cambia nulla 

    int deviceIndex = supportSemAsid[asidTerminate-1]; // rilasciamo
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
    dtpreg_t* devReg = (dtpreg_t*)DEV_REG_ADDR(IL_PRINTER, devNo);
    unsigned int status;
    int charSent = 0;
    while (charSent < length) { // loop di un carattere alla volta; come visto in print di p2test
        devReg->data0 = (unsigned int)*msg;
        status = SYSCALL(DOIO, (int)&devReg->command, TRANSMITCHAR, 0);
        if ((status & 0xFF) != READY) {
            devReg->status = -status;
        }
        msg++;  
        charSent++;
    }
    return charSent;
}

void writeDevice(state_t *stato, int asid, int type){
    char* vAddrMSG = (char*)stato->reg_a1;
    int str_len = stato->reg_a2;

    if((unsigned int)vAddrMSG < UPROCSTARTADDR || ((unsigned int)vAddrMSG + str_len) > USERSTACKTOP || str_len < 0 || str_len > MAXSTRLENG) {
        TerminateSYS(asid);
    }

    int deviceNo = asid - 1;
    int status = 0;

    if(type == WRITETERMINAL) {
        termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, deviceNo);
        memaddr* cmdAddr = (memaddr*)&devReg->transm_command;
        int deviceIndex = findDevice(cmdAddr);

        if(deviceIndex < 0 || deviceIndex >= 48) {
            print("DEVICE SBAGLIATO\n");
            stato->reg_a0 = -1;
            stato->pc_epc += 4;
            LDST(stato);
        }

        acquireDevice(asid, deviceIndex);
        status = printTerminal(vAddrMSG, str_len, deviceNo);
        releaseDevice(asid, deviceIndex);

    } else if(type == WRITEPRINTER) {
        dtpreg_t* devReg = (dtpreg_t*)DEV_REG_ADDR(IL_PRINTER, deviceNo);
        memaddr* cmdAddr = (memaddr*)&devReg->command;
        int deviceIndex = findDevice(cmdAddr);

        if(deviceIndex < 0 || deviceIndex >= 48) {
            stato->reg_a0 = -1;
            stato->pc_epc += 4;
            print("DEVICE SBAGLIATO\n");
            LDST(stato);
        }

        acquireDevice(asid, deviceIndex);
        status = printPrinter(vAddrMSG, str_len, deviceNo);
        releaseDevice(asid, deviceIndex);
    }

    stato->reg_a0 = (status < 0 ? status : str_len);
    stato->pc_epc += 4;
    LDST(stato);
}


// riceve da terminale e salva su address
int inputFromTerminal(char* addr, int term) {
    termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, term);
    int str_len = 0;
    while (1) { // legge finché non \n!!!
        
        int status = SYSCALL(DOIO, (int)&devReg->recv_command, RECEIVECHAR, 0);
        if ((status & 0xFF) != CHARRECV) {
            return -status;
        }
        char character = (char)(status >> 8); // prende il carattere, shiftato di 8 || guardare il print per capire come funziona e lo salva in addr
        *addr = character;
        addr++;
        str_len++;
        if (character == '\n') break;
    }
    return str_len;
}
void readTerminal(state_t* stato, int asid){
    char *vAddr = (char *) stato->reg_a1; 
    if((memaddr)vAddr <  KUSEG || (memaddr) vAddr >= USERSTACKTOP) {
        TerminateSYS(asidAcquired);
    }

    int devNo = asid-1;
    termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, devNo);
    memaddr* cmdAddr = &devReg->recv_command;
    int deviceIndex = findDevice(cmdAddr);
    if(deviceIndex < 0 || deviceIndex >= 48) {
        print("DEVICE SBAGLIATO\n");
            stato->reg_a0 = -1;
            stato->pc_epc += 4;
            LDST(stato);
        }
    acquireDevice(asid, deviceIndex);
    int status = inputFromTerminal(vAddr, devNo);
    releaseDevice(asid, deviceIndex);

    stato->reg_a0 = status;
    stato->pc_epc += 4; 
    LDST(stato);
}


void generalExceptionHandler(){
    support_t *sup = (support_t *) SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    // determiniamo la causa
    state_t* state = &(sup->sup_exceptState[GENERALEXCEPT]);

    //Decodifica l'eccezione: se NON è SYSCALL -> Program Trap 
    unsigned int exccode = (state->cause & GETEXECCODE);  // mask/shift definiti in const.h

    klog_print("generalExceptionHandler: exccode=");
    klog_print_dec(exccode);
    klog_print(", reg_a0=");
    klog_print_dec(state->reg_a0);
    klog_print(", asid=");
    klog_print_dec(sup->sup_asid);
    klog_print("\n");
    if (exccode != SYSEXCEPTION) {
        supportTrapHandler(sup->sup_asid);   // Program Trap handler 
        return;                    // non proseguire nel dispatch SYSCALL       //AGGIUNTO
    }
    //incremento così passo all'istruzione dopo la syscall (cap7)
    //state->pc_epc += 4; //incremento di 4 il program counter che ha causato l'eccezione
    int asid = sup->sup_asid;
    switch (state->reg_a0){
        case TERMINATE:   // SYS2
//          
            TerminateSYS(asid);
            break;

        case WRITEPRINTER: // SYS3
//              klog_print(" WRITEPRINTER "); 
//            klog_print_dec(asid);
            writeDevice(state, asid, WRITEPRINTER);
            break;

        case WRITETERMINAL: // SYS4
//              klog_print(" WRITETERMINAL "); 
//            klog_print_dec(asid);
            writeDevice(state, asid, WRITETERMINAL);
            break;

        case READTERMINAL:  // SYS5
//             klog_print(" READTERMINAL "); 
//            klog_print_dec(asid);
            readTerminal(state, asid); 
            break;

        default:
//              klog_print(" TRAP "); 
//            klog_print_dec(asid);
            supportTrapHandler(sup->sup_asid);
            break;
    }
}



void supportTrapHandler(int asid){    
  TerminateSYS(asid);
}
