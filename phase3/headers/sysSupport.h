#ifndef SYS_SUPPORT_H
#define SYS_SUPPORT_H
#include "initProc.h"
#include "vmSupport.h"

void generalExceptionHandler();
void supportTrapHandler(support_t sup_ptr);      //definito qui (usato con extern in vmSupport)
#endif