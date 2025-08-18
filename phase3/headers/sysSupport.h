#ifndef SYS_SUPPORT_H
#define SYS_SUPPORT_H
#include "../../headers/const.h"
#include "../../headers/types.h"
void generalExceptionHandler();
void supportTrapHandler(support_t sup_ptr);      //definito qui (usato con extern in vmSupport)
#endif