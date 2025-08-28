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

extern void klog_print(const char *str);
extern void klog_print_dec(unsigned int num);
extern int masterSem;
// TerminateSYS: termina il processo con ASID asidTerminate
void TerminateSYS(int asidTerminate) {
    // Se non ho già acquisito il mutex, lo prendo
    if (asidAcquired != asidTerminate) {
        acquire_mutexTable(asidTerminate);
    }

    // Pulisco la swap pool dei frame appartenenti al processo
    for (int i = 0; i < POOLSIZE; i++) {
        swap_t *swap = &swap_pool[i];
        if (swap->sw_asid == asidTerminate) {
            swap->sw_asid = -1;
        }
    } 
    release_mutexTable(); // Rilascio il mutex della swap pool

    // Rilascia eventuale device acquisito dal processo
    int deviceIndex = supportSemAsid[asidTerminate-1];
    if (deviceIndex != -1) {
        releaseDevice(asidTerminate, deviceIndex);
    }

    // Faccio una V sul masterSem per notificare la terminazione
    SYSCALL(VERHOGEN, (int)&masterSem, 0, 0);
    // Termino il processo corrente
    SYSCALL(TERMINATE, 0, 0, 0);
}

// Funzioni per la scrittura su dispositivi
int printTerminal(char* msg, int lenMsg, int term) {
    termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, term);
    unsigned int status;
    int charSent = 0;
    while (charSent < lenMsg) { // loop di un carattere alla volta
        unsigned int value = PRINTCHR | (((unsigned int)*msg) << 8);
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
    while (charSent < length) { // loop di un carattere alla volta
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

// inputFromTerminal: legge stringa da terminale e la salva in addr
int inputFromTerminal(char* addr, int term) {
    termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, term);
    int str_len = 0;
    while (1) { // legge finché non \n
        int status = SYSCALL(DOIO, (int)&devReg->recv_command, RECEIVECHAR, 0);
        if ((status & 0xFF) != CHARRECV) {
            return -status;
        }
        char character = (char)(status >> 8);
        *addr = character;
        addr++;
        str_len++;
        if (character == '\n') break;
    }
    return str_len;
}

// writeDevice: scrive su terminale o printer
void writeDevice(state_t *stato, int asid, int type){
    char* vAddrMSG = (char*)stato->reg_a1;
    int str_len = stato->reg_a2;

    if((unsigned int)vAddrMSG < UPROCSTARTADDR || 
       ((unsigned int)vAddrMSG + str_len) > USERSTACKTOP || 
       str_len < 0 || str_len > MAXSTRLENG) {
        TerminateSYS(asid);
    }

    int deviceNo = asid - 1;
    int status = -1;
    int deviceIndex;

    if(type == WRITETERMINAL) {
        termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, deviceNo);
        memaddr* cmdAddr = (memaddr*)&devReg->transm_command;
        deviceIndex = findDevice(cmdAddr);

        if(deviceIndex != -1){
            acquireDevice(asid, deviceIndex);
            status = printTerminal(vAddrMSG, str_len, deviceNo);
            releaseDevice(asid, deviceIndex);
        }

    } else if(type == WRITEPRINTER) {
        dtpreg_t* devReg = (dtpreg_t*)DEV_REG_ADDR(IL_PRINTER, deviceNo);
        memaddr* cmdAddr = (memaddr*)&devReg->command;
        deviceIndex = findDevice(cmdAddr);

        if(deviceIndex != -1){
            acquireDevice(asid, deviceIndex);
            status = printPrinter(vAddrMSG, str_len, deviceNo);
            releaseDevice(asid, deviceIndex);
        }
    }

    stato->reg_a0 = (status < 0 ? status : str_len);
    stato->pc_epc += 4; // incremento PC sempre
    LDST(stato);
}

// readTerminal: legge dal terminale e aggiorna il processo
void readTerminal(state_t* stato, int asid){
    char *vAddr = (char *) stato->reg_a1; 

    if((memaddr)vAddr <  KUSEG || (memaddr) vAddr >= USERSTACKTOP) {
        TerminateSYS(asid);
    }

    int devNo = asid - 1;
    termreg_t* devReg = (termreg_t*)DEV_REG_ADDR(IL_TERMINAL, devNo);
    memaddr* cmdAddr = &devReg->recv_command;
    int deviceIndex = findDevice(cmdAddr);
    int status = -1;

    if(deviceIndex != -1){
        acquireDevice(asid, deviceIndex);
        status = inputFromTerminal(vAddr, devNo);
        releaseDevice(asid, deviceIndex);
    }

    stato->reg_a0 = status;
    stato->pc_epc += 4; 
    LDST(stato);
}

// generalExceptionHandler: gestisce tutte le eccezioni generali
void generalExceptionHandler(){
    support_t *sup = (support_t *) SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    state_t* state = &(sup->sup_exceptState[GENERALEXCEPT]);

    unsigned int exccode = state->cause & GETEXECCODE;

    if (exccode != SYSEXCEPTION) {
        supportTrapHandler(sup->sup_asid);   // Program Trap handler 
        return;
    }

    int asid = sup->sup_asid;

    switch (state->reg_a0){
        case TERMINATE:        // SYS2
            TerminateSYS(asid);
            break;

        case WRITEPRINTER:     // SYS3
            writeDevice(state, asid, WRITEPRINTER);
            break;

        case WRITETERMINAL:    // SYS4
            writeDevice(state, asid, WRITETERMINAL);
            break;

        case READTERMINAL:     // SYS5
            readTerminal(state, asid); 
            break;

        default:
            supportTrapHandler(asid);
            break;
    }
}

// supportTrapHandler: wrapper per terminare il processo su trap non gestita
void supportTrapHandler(int asid){    
    TerminateSYS(asid);
}

