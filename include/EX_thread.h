#ifndef __EX_THREAD_H
#define __EX_THREAD_H

#include "sys_core.h"

typedef struct exInfo
{
	uint32_t PC;
	uint32_t immediate;
	opcodes opcode;
	uint32_t Rs;
	uint32_t Rt;
	uint32_t updatedPcVal;
	bool updatePC;
}exInfo_t, *exInfoPtr_t;

void EXthread(SysCore& core);

#endif