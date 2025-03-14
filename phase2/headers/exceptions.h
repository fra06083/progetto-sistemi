#ifndef EXC_H_INCLUDED
#define EXC_H_INCLUDED

#include "../../headers/listx.h"
#include "../../headers/types.h"
#include <uriscv/cpu.h>
#include <uriscv/arch.h>
#include <uriscv/types.h>
#include <uriscv/liburiscv.h> // libreria di uriscv, viene richiesta, sennò non abbiamo le funzioni che ci servono LDST

// Funzioni esterne
extern unsigned int getPRID(void);
extern unsigned int setENTRYHI(unsigned int entry);
extern unsigned int setENTRYLO(unsigned int entry);
extern unsigned int LDST(void* state);
extern void uTLB_RefillHandler();
// Funzioni dichiarate
void exceptionHandler();
void syscallHandler();
#endif
