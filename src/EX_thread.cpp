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
		case LDW: case STW: return operandA + operandB;

		//control flow instructions
		case BZ: case BEQ: case JR: case HALT: case INVALID: break;
	}
	return 0;
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

/* 
* Description: 
*	Interpret Control Flow operations and update PC if necessary.
*
* Arguments:
*	(INPUT) PC -- uint32_t value, current PC
*	(INPUT) instructionData -- instInfoPtr_t current instruction info
*
* Return:
*	long long -- new PC value or -1 if PC was not updated
*/
long long updatePC (uint32_t PC, instInfoPtr_t instructionData){
	long long newPcVal;
	uint32_t instruction;
	instruction = (PC - 8) / 4;
	switch(instructionData->opcode){
		case BZ:	
			if (instructionData->RsValHolder == 0) {
				newPcVal = ((instruction + signExtend(instructionData->immediateValHolder)) * 4); 
				std::cout << "New CALC PC in EX: " << newPcVal << "\n";
			}
			else {
				newPcVal = -1;
			}
			break;
		case BEQ:
			if(instructionData->RsValHolder == instructionData->RtValHolder){
				newPcVal = ((instruction + signExtend(instructionData->immediateValHolder)) * 4); 
				std::cout << "New CALC PC in EX: " << newPcVal << "\n";
			}
			else {
				newPcVal = -1;
			}
			break;
		case JR:
			newPcVal  = instructionData->RsValHolder;
			std::cout << "New CALC PC in EX: " << newPcVal << "\n";
			break;
		case HALT: break;
		default:
			newPcVal = -1;	//signal PC was not updated
			break;
	}
	return newPcVal;
}

void EXthread(SysCore& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::milliseconds delay(MIN_SLEEP_TIME);
	instInfoPtr_t instructionData;
	//exInfoPtr_t exInfo = new exInfo_t;

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

			uint32_t currentPC = sysCore.PC;	//current PC for use in control flow ops
			std::cout << "Current PC in EX: " << currentPC << "\n";
			uint32_t extendedImm;				//sign extended immediate
			long long updatedPcVal;				//updated PC value			
	
			//If R type instruction perform operation and store in RD val holder
			if (instructionData->type == Rtype) {
				instructionData->RdValHolder = alu(instructionData->RsValHolder, instructionData->RtValHolder, instructionData->opcode);
			}
			//Perform I type operations 
			else if (instructionData->type == Itype) {
				
				//sign extend immediate into 32 bit val
				extendedImm = signExtend(instructionData->immediateValHolder);
				
				//control flow ops: caclulate new PC value if applicable. 
				if (instructionData->isControlFlow == true) {
					if ((updatedPcVal = updatePC(currentPC, instructionData)) != -1) {
						sysCore.stageInfoEX.fwdedAluResult = static_cast<uint32_t>(updatedPcVal);
						sysCore.stageInfoEX.updatedPC = true;

                        // Report to master thread that we have a branch taken
                        std::cout <<"DEBUG: [EXthread] Branch Taken\n";
                        sysCore.stageInfoEX.errorType = errorCodes::ERR_BRANCH_TAKEN;
					} 
					else {
						sysCore.stageInfoEX.updatedPC = false;
					}	
				}
				//calculate effecive mem address if mem access op
				else if (instructionData->opcode == LDW || instructionData->opcode == STW) {
					instructionData->memAddressValHolder = alu(instructionData->RsValHolder, extendedImm, instructionData->opcode);
					//std::cout << "EX mem addr =  " << instructionData->memAddressValHolder << '\n';
				}
				//if arithmetic/logical immediate op perform operation and store in RT val holder
				else {
					instructionData->RtValHolder = alu(instructionData->RsValHolder, extendedImm, instructionData->opcode);
				}	
			}
			else
				std::cerr << "\nERROR: ALU encountered unknown instruction type\n" << std::endl;
			
			#if (_VERBOSE_ > 0)
				std::cout << "Opcode =  " << instructionData->opcode << '\n';
				std::cout << "Rd Result =  " << instructionData->RdValHolder << '\n';
				std::cout << "Rs =  " << instructionData->RsValHolder << '\n';
				std::cout << "Rt =  " << instructionData->RtValHolder << '\n';
				std::cout << "Immediate =  " << std::bitset<32>(instructionData->immediateValHolder) << '\n';
				std::cout << "Updated PC =  " << updatedPcVal << '\n';
			#endif

	
			/* Still need to update/work on this logic*/
			// sysCore.stageInfoEX.fwdedImmediate = instructionData->immediateValHolder;
			// sysCore.stageInfoEX.fwdedRd = instructionData->RdValHolder;
			// sysCore.stageInfoEX.fwdedRs = instructionData->RsValHolder;
			// sysCore.stageInfoEX.fwdedRt = instructionData->RtValHolder;
			

			//Pass alu data to MEM stage (will block if it cannot immediately acquire the lock)
			sysCore.EXtoMEM.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
