#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "EX_thread.h"
#include <bitset>


#define MIN_SLEEP_TIME		50  //In ms
#define _VERBOSE_ 0
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
		case LDW: case STW: return (operandA + operandB) / 4;

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
uint32_t signExtend(uint16_t immediate16_t){
	uint32_t immediate32_t;	
	if(immediate16_t & BITMASK){
        immediate32_t = immediate16_t | SIGNEXTEND;
	} 
	else{
		immediate32_t = immediate16_t;
	}
	#if (_VERBOSE_ == 2)
		std::cout << "Before sign extend Immediate =  " << std::bitset<16>(immediate16_t) << '\n';
		std::cout << "In sign extend Immediate =  " << std::bitset<32>(immediate32_t) << '\n';		
	#endif
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
			exInfo->opcode = instructionData->opcode;
			exInfo->PC = sysCore.PC;
			exInfo->Rs = instructionData->RsValHolder;
			exInfo->Rt = instructionData->RtValHolder;
			exInfo->updatedPcVal = 0;
			exInfo->updatePC = false;
			
			//perform alu op or update PC depending on instruction type
			if(instructionData->type == Itype) {
				
				exInfo->immediate = signExtend(instructionData->immediateValHolder);
				
				//control flow ops: caclulate new PC value is applicable. 
				if(instructionData->isControlFlow == true) {
					updatePC(exInfo);
					instructionData->aluResultHolder =	exInfo->updatedPcVal; //store new PCval in aluResultHolder
				}
				//if Immediate type: perform operation and store in RT val holder
				else {
					instructionData->RtValHolder = alu(exInfo->Rs, exInfo->immediate, exInfo->opcode);
				}	
			}
			//R type: perform operation and store in RD val holder
			else if (instructionData->type == Rtype) {
				instructionData->RdValHolder = alu(instructionData->RsValHolder, instructionData->RtValHolder, instructionData->opcode);
			}
			else
				std::cerr << "\nERROR: ALU encountered unknown instruction type\n" << std::endl;
			
			#if (_VERBOSE_ > 0)
				std::cout << "Opcode =  " << exInfo->opcode << '\n';
				std::cout << "Rd Result =  " << instructionData->RdValHolder << '\n';
				std::cout << "Rs =  " << exInfo->Rs << '\n';
				std::cout << "Rt =  " << instructionData->RtValHolder << '\n';
				std::cout << "Immediate =  " << std::bitset<32>(exInfo->immediate) << '\n';
				std::cout << "Updated PC =  " << exInfo->updatedPcVal << '\n';
			#endif
		
			//sysCore.stageInfoEX.fwdedAluResult update if jump/branch target PC result
			sysCore.stageInfoEX.fwdedAluResult = instructionData->aluResultHolder;
			sysCore.stageInfoEX.fwdedImmediate = instructionData->immediateValHolder;
			sysCore.stageInfoEX.fwdedRd = instructionData->RdValHolder;
			sysCore.stageInfoEX.fwdedRs = instructionData->RsValHolder;
			sysCore.stageInfoEX.fwdedRt = instructionData->RtValHolder;
			sysCore.stageInfoEX.updatedPC = exInfo->updatePC;

			//Pass alu data to MEM stage (will block if it cannot immediately acquire the lock)
			sysCore.EXtoMEM.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
