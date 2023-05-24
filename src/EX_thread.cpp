#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "EX_thread.h"


#define MIN_SLEEP_TIME		50  //In ms

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

void updatePC (exInfoPtr_t instrData){
	
	switch(instrData->opcode){
		case BZ:
			if (instrData->Rs == 0) {
				instrData->updatedPcVal = instrData->PC + instrData->immediate; 
				instrData->updatePC = true;
			}
			break;
		case BEQ:
			if(instrData->Rs == instrData->Rt){
				instrData->updatedPcVal = instrData->PC + instrData->immediate; 
				instrData->updatePC = true;
			}
			break;
		case JR:
			instrData->updatedPcVal = instrData->Rs;
			instrData->updatePC = true;
			break;
		case HALT: break;
		default:
			instrData->updatedPcVal = 0;
			instrData->updatePC = false;
			break;
	}
}

/*Perform Sign Extension on 16 bit immediates*/
uint32_t signExtend(u_int16_t immediate16_t){
	uint32_t immediate32_t;	
	if(immediate16_t & 0x8000){
        immediate32_t = immediate16_t | 0xFFFF0000;
	} 
	else{
		immediate32_t = immediate16_t;
	}
	return immediate32_t;
}

void EXthread(SysCore& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::milliseconds delay(MIN_SLEEP_TIME);
	instInfoPtr_t instructionData;
	exInfoPtr_t exInfo = new exInfo_t;

	while (true)
	{		
		
		//See if we need to die or not
		if (sysCore.stageInfoEX.die) {
			return;
		}

		//Only coninue if the clock has changed and we have the go ahead from the master
		if (pastClkVal < sysCore.clk && sysCore.stageInfoEX.okToRun)
		{	
			//Clear our flag
			sysCore.stageInfoEX.okToRun = false;

			//Try to get instruction out of the queue (will block if it cannot immediately acquire the lock)
			instructionData = sysCore.IDtoEX.pop();


			//If there was nothing for us to get, we missed our opportunity for this clock. Reset
			if (instructionData == NULL)
			{
				std::cout << "DEBUG: [EXthread] Missed opportunity for this clock, will try again next clock" << std::endl;
				continue;
			}

			//Skip the data if its invalid (aka we are told to flush)
			if (sysCore.stageInfoEX.invalidateData)
			{
				std::cout << "DEBUG: [EXthread] Told to invalidate instuction, will try again next clock" << std::endl;
				sysCore.stageInfoEX.invalidateData = false;
				continue;
			}

			//Record the new clock value
			pastClkVal = sysCore.clk;

			//Initialize execution stage info struct
			exInfo->immediate = signExtend(instructionData->immediateValHolder);
			exInfo->opcode = instructionData->opcode;
			exInfo->PC = sysCore.PC;
			exInfo->Rs = instructionData->RsValHolder;
			exInfo->Rt = instructionData->RsValHolder;
			exInfo->updatedPcVal = NULL;
			exInfo->updatePC = false;
			
			

	
			switch(instructionData->type)
			{
				case Rtype: 
					instructionData->aluResultHolder = alu(instructionData->RsValHolder, instructionData->RtValHolder, instructionData->opcode);
					break;
				case Itype:
					instructionData->aluResultHolder = alu(instructionData->RsValHolder, instructionData->immediateValHolder, instructionData->opcode);
					break;
				default:
					std::cerr << "\nERROR: ALU encountered unknown instruction type\n" << std::endl;
					break;
			}

			/*TO DO: all work in progress. still trying to determine
			how to handle ALU results based on instruction type. 
			-Should arithmetic/logical results be written to Destination register in this thread?
				--currently writing to a fwdedAluResult var in struct to pass value forward.
			-Should calculated load/store address temp be stored in destination register in this thread?
			-Should branching instructions update PC / set any flags in this thread?
			*/

		//sysCore.stageInfoEX.fwdedAluResult update if jump/branch target PC result

			//Pass alu data to MEM stage (will block if it cannot immediately acquire the lock)
			sysCore.EXtoMEM.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
