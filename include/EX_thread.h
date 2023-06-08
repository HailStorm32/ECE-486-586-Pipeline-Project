#ifndef __EX_THREAD_H
#define __EX_THREAD_H

#include "sys_core.h"

#define BITMASK 0x8000
#define SIGNEXTEND 0xFFFF0000

void EXthread(SysCore& core);
uint32_t alu (uint32_t operandA, uint32_t operandB, opcodes operation);
long long updatePC (uint32_t PC, instInfoPtr_t instructionData);
uint32_t signExtend(uint16_t immediate16_t);

#endif