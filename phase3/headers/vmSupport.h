#ifndef EXC_VM_INCLUDED
#define EXC_VM_INCLUDED
// Funzioni dichiarate
void uTLB_ExceptionHandler();
void acquire_mutexTable(int asid);
void release_mutexTable();
#endif
