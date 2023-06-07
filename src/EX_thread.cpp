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
	instruction = PC/4;
	switch(instructionData->opcode){
		case BZ:	
			if (instructionData->RsValHolder == 0) {
				newPcVal = ((instruction + signExtend(instructionData->immediateValHolder)) * 4); 
				std::cout << "New CALC PC in BZ: " << newPcVal << "\n";
			}
			else {
				newPcVal = -1;
			}
			break;
		case BEQ:
			if(instructionData->RsValHolder == instructionData->RtValHolder){
				newPcVal = ((instruction + signExtend(instructionData->immediateValHolder)) * 4); 
				std::cout << "*****************New CALC PC in BEQ***********: " << newPcVal << "\n";
			}
			else {
				newPcVal = -1;
			}
			break;
		case JR:
			newPcVal  = instructionData->RsValHolder;
			std::cout << "New CALC PC in JR: " << newPcVal << "\n";
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
	bool forwardData = false;

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
				//std::cout << "DEBUG: [EXthread] Missed opportunity for this clock, will try again next clock" << std::endl;
				continue;
			}

			//Skip the data if its invalid (aka we are told to flush)
			if (sysCore.stageInfoEX.invalidateData)
			{
				//std::cout << "DEBUG: [EXthread] Told to invalidate instuction, will try again next clock" << std::endl;
				sysCore.stageInfoEX.invalidateData = false;

				//We no longer need the instruction data, delete it
				delete instructionData;
				continue;
			}

			//Record the new clock value
			pastClkVal = sysCore.clk;

			
			uint32_t currentPC = instructionData->instrPC;
			uint32_t extendedImm;				//sign extended immediate
			long long updatedPcVal;				//updated PC value			

			//See if there is a forward request for the current instruction
			if (sysCore.stageInfoEX.useFwdHashTable.count(instructionData->generatedID)) {

				if (sysCore.stageInfoEX.useFwdHashTable[instructionData->generatedID].fwdedFrom != fowardInfo::NONE) {
					//TODO: add code to revieve forwarded values here
				}

				if (sysCore.stageInfoEX.useFwdHashTable[instructionData->generatedID].fwdTo != fowardInfo::NONE) {
					forwardData = true;
				}
			}
	
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
						sysCore.stageInfoIF.fwdedAluResult = static_cast<uint32_t>(updatedPcVal);

                        // Report to master thread that we have a branch taken
                        std::cout <<"DEBUG: [EXthread] Branch Taken\n";
                        sysCore.stageInfoEX.errorType = errorCodes::ERR_BRANCH_TAKEN;
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
				std::cerr << "\nERROR: [EXthread] ALU encountered unknown instruction type\n" << std::endl;
			
			#if (_VERBOSE_ > 0)
				std::cout << "Opcode =  " << instructionData->opcode << '\n';
				std::cout << "Rd Result =  " << instructionData->RdValHolder << '\n';
				std::cout << "Rs =  " << instructionData->RsValHolder << '\n';
				std::cout << "Rt =  " << instructionData->RtValHolder << '\n';
				std::cout << "Immediate =  " << std::bitset<32>(instructionData->immediateValHolder) << '\n';
				std::cout << "Updated PC =  " << updatedPcVal << '\n';
			#endif

			//Forward data if we are supposed to
			if (forwardData) {
				//Get the consumer instruction ID (ie the instruction that needs the data)
				uint32_t consumerInstID = sysCore.stageInfoEX.useFwdHashTable[instructionData->generatedID].consumerInstID;

				switch (sysCore.stageInfoEX.useFwdHashTable[instructionData->generatedID].fwdTo){

				case fowardInfo::IF:
					std::cerr << "\nWARNING: [EXthread] Told to forward to IF when code doesnt exist, skipping..\n";
					break;

				case fowardInfo::ID:
					//Determine if the current isntruction is R or I type, as that will determine what we forward
					switch (instructionData->type)
					{
					case instFormat::Rtype:
						//Forward the Rd data to the ID stage
						sysCore.stageInfoID.useFwdHashTable[consumerInstID].fwdedRd = instructionData->RdValHolder;
						break;

					case instFormat::Itype:
						//Forward the Rt data to the ID stage
						sysCore.stageInfoID.useFwdHashTable[consumerInstID].fwdedRt = instructionData->RtValHolder;
						break;

					default:
						std::cerr << "\nERROR: [EXthread] Told to forward to ID but instruction type is unknown\n";
						break;
					}
					break;

				case fowardInfo::EX:
					std::cerr << "\nWARNING: [EXthread] Told to forward to EX when code doesnt exist, skipping..\n";
					break;

				case fowardInfo::MEM:
					std::cerr << "\nWARNING: [EXthread] Told to forward to MEM when code doesnt exist, skipping..\n";
					break;

				case fowardInfo::WB:
					std::cerr << "\nWARNING: [EXthread] Told to forward to WB when code doesnt exist, skipping..\n";
					break;

				default:
					std::cerr << "\nWARNING: [EXthread] Given invalid destination to forward to, skipping..\n";
					break;
				}

				//Removed the forward request from the hash table
				sysCore.stageInfoEX.useFwdHashTable.erase(instructionData->generatedID);

				//Reset the forward flag
				forwardData = false;
			}
			
			//Pass alu data to MEM stage (will block if it cannot immediately acquire the lock)
			sysCore.EXtoMEM.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
