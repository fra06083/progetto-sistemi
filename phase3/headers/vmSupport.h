#ifndef EXC_VM_INCLUDED
#define EXC_VM_INCLUDED
#include "../../headers/const.h"
#include "../../headers/types.h"
#include "uriscv/arch.h"
#include "uriscv/cpu.h"
// Funzioni dichiarate
void uTLB_ExceptionHandler();
void acquire_mutexTable(int asid);
void release_mutexTable();
#endif
