#ifndef __EX_THREAD_H
#define __EX_THREAD_H

#include "sys_core.h"

#define BITMASK 0x8000
#define SIGNEXTEND 0xFFFF0000

void EXthread(SysCore& core);

#endif