#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "EX_thread.h"


#define MIN_SLEEP_TIME		200  //In ms

/* 
* Description: *****WORK IN PROGRESS******** 
*	Perform ALU functions.
*
* Arguments:
*	(INPUT) operandA -- uint32_t value, source reg RS
*	(INPUT) operandB -- uint32_t value, source varies by inst type
*	(INPUT) operation -- enum opcodes value, source from instruction opcode
* Return:
*	uint32_t -- value calculated by ALU
*/
uint32_t alu (uint32_t operandA, uint32_t operandB, opcodes operation){

	switch (operation){
		//arithmetic and logical operations
		case ADD: case ADDI: return operandA + operandB;
		case SUB: case SUBI: return operandA - operandB;
		case MUL: case MULI: return operandA * operandB;
		case OR: case ORI: return operandA | operandB;
		case AND: case ANDI: return operandA & operandB;
		case XOR: case XORI: return operandA ^ operandB;

		//Memory Access operations
		case LDW: case STW: return operandA + operandB;

		//control flow instructions
		case BZ: case BEQ: case JR: case HALT: case INVALID: break;
	}
	return 0;
}

void EXthread(SysCore& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::milliseconds delay(MIN_SLEEP_TIME);
	instInfoPtr_t instructionData;

	while (true)
	{		
		
		//Only coninue if the clock has changed and we have the go ahead from the master
		if (pastClkVal < sysCore.clk && sysCore.stageInfoEX.okToRun)
		{	
			std::cout <<"In EX thread\n";
			//Try to get instruction out of the queue (will block if it cannot immediately acquire the lock)
			instructionData = sysCore.IDtoEX.pop();


			//If there was nothing for us to get, we missed our opportunity for this clock. Reset
			if (instructionData == NULL)
			{
				std::cout << "DEBUG: [EXthread] Missed opportunity for this clock, will try again next clock" << std::endl;
				sysCore.stageInfoEX.okToRun = false;
				continue;
			}

			//Record the new clock value
			pastClkVal = sysCore.clk;

	
			switch(instructionData->type)
			{
				case Rtype: 
					instructionData->aluResult = alu(instructionData->RsVal, instructionData->RtVal, instructionData->opcode);
					break;
				case Itype:
					instructionData->aluResult = alu(instructionData->RsVal, instructionData->immediate, instructionData->opcode);
					break;
				default:
					std::cerr << "\nERROR: ALU encountered unknown instruction type\n" << std::endl;
					break;
			}

			/*TO DO: all work in progress. still trying to determine
			how to handle ALU results based on instruction type. 
			-Should arithmetic/logical results be written to Destination register in this thread?
				--currently writing to a aluResult var in struct to pass value forward.
			-Should calculated load/store address temp be stored in destination register in this thread?
			-Should branching instructions update PC / set any flags in this thread?
			*/

		//sysCore.stageInfoEX.aluResult update if jump/branch target PC result

			//Pass alu data to MEM stage (will block if it cannot immediately acquire the lock)
			sysCore.EXtoMEM.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
