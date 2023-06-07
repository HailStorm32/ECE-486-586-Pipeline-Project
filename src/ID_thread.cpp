#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "ID_thread.h"
#include "decoder.h"

#define MIN_SLEEP_TIME		50  //In ms

void IDthread(SysCore& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::milliseconds delay(MIN_SLEEP_TIME);
	instPreInfoPtr_t instPreInfoPkg;
	uint32_t forwardedValue = 0;
	instRegTypes valNeeded = instRegTypes::REG_NONE;

	while (true)
	{		
		//See if we need to die or not
		if (sysCore.stageInfoID.die) {
			return;
		}

		//Only coninue if the clock has changed and we have the go ahead from the master
		if (pastClkVal < sysCore.clk && sysCore.stageInfoID.okToRun)
		{
			//Clear our flag
			sysCore.stageInfoID.okToRun = false;

			//Try to get instruction out of the queue (will block if it cannot immediately acquire the lock)
			instPreInfoPkg = sysCore.IFtoID.pop();

			//If there was nothing for us to get, we missed our opportunity for this clock. Reset
			if (instPreInfoPkg == NULL)
			{
				std::cout << "DEBUG: [IDthread] Missed opportunity for this clock, will try again next clock" << std::endl;
				continue;
			}

			//Skip the data if its invalid (aka we are told to flush)
			if (sysCore.stageInfoID.invalidateData)
			{
				std::cout << "DEBUG: [IDthread] Told to Invalidate Data" << std::endl;
				sysCore.stageInfoID.invalidateData = false;
				continue;
			}

			//Record the new clock value
			pastClkVal = sysCore.clk;

			//Decode the instruction
			instInfoPtr_t instructionData = decodeInstruction(instPreInfoPkg->rawInstruction);
			
			//update instruction PC value
			instructionData->instrPC = instPreInfoPkg->instructionPC;

			if (instructionData == NULL)
			{
				std::cerr << "\nERROR: Invalid instruction: 0x" << std::hex << instPreInfoPkg->rawInstruction << std::dec << ", Skipping...\n\n";
				//TODO: Report to master thread that we have an invalid instruction
				sysCore.stageInfoID.errorType = errorCodes::ERR_INVALID_INST;
				continue;
			}

			switch (instructionData->opcode)
			{
				case opcodes::ADD: case opcodes::SUB: case opcodes::MUL:case opcodes::ADDI: case opcodes::SUBI: case opcodes::MULI:
					sysCore.instrCountStruct.arithmeticCount += 1;
					break;
				case opcodes::OR: case opcodes::AND: case opcodes::XOR: case opcodes::ORI: case opcodes::ANDI: case opcodes::XORI: 
					sysCore.instrCountStruct.logicalCount += 1;
					break;
				case opcodes::LDW: case opcodes::STW: 
					sysCore.instrCountStruct.memAccesCount += 1;
					break;
				case opcodes::BZ: case opcodes::BEQ: case opcodes::JR: case opcodes::HALT:
					sysCore.instrCountStruct.controlTransferCount += 1;
					break;
				default:
					std::cerr << "\nERROR: can't ID instruction type" << std::endl;
				break;
			}

			//Report if we found a HALT
			if (instructionData->opcode == opcodes::HALT)
			{
				sysCore.stageInfoID.errorType = errorCodes::ERR_HALT;
				delete instructionData;
				delete instPreInfoPkg;
				continue;
			}

			//Write the ID
			instructionData->generatedID = instPreInfoPkg->generatedID;
			
			delete instPreInfoPkg;
			
			//See if there is a forward request for the current instruction
			if (sysCore.stageInfoID.useFwdHashTable.count(instructionData->generatedID))
			{
				//Get the forwarded value
				switch (sysCore.stageInfoID.useFwdHashTable[instructionData->generatedID].fwdedRegister)
				{
				case instRegTypes::Rd:
					forwardedValue = sysCore.stageInfoID.useFwdHashTable[instructionData->generatedID].fwdedRd;
					break;
				case instRegTypes::Rt:
					forwardedValue = sysCore.stageInfoID.useFwdHashTable[instructionData->generatedID].fwdedRt;
					break;
				default:
					std::cerr << "\nERROR: [IDthread] given invalid forward register to read from\n\n";
					break;
				}

				//Figure out the type of instruction we are getting values for
				switch (instructionData->type)
				{
				case instFormat::Itype:
					valNeeded = sysCore.stageInfoID.useFwdHashTable[instructionData->generatedID].regValNeeded;

					if (valNeeded == instRegTypes::Rs)
					{
						//Get the forwarded value
						instructionData->RsValHolder = forwardedValue;

						//Get remaining values the normal way
						instructionData->RtValHolder = sysCore.reg[instructionData->RtAddr];
						instructionData->RdValHolder = sysCore.reg[instructionData->RdAddr];
					}
					//Special case for BEQ instruction 
					else if (instructionData->opcode == opcodes::BEQ && valNeeded == instRegTypes::Rt)
					{
						//Get the forwarded value
						instructionData->RtValHolder = forwardedValue;

						//Get remaining values the normal way
						instructionData->RsValHolder = sysCore.reg[instructionData->RsAddr];
						instructionData->RdValHolder = sysCore.reg[instructionData->RdAddr];
					}
					else
					{
						std::cerr << "\nERROR: [IDthread] given invalid forward register to read from\n\n";
					}

					break;

				case instFormat::Rtype:
					
					//Figure out what register the forwarded value gets written to
					switch (sysCore.stageInfoID.useFwdHashTable[instructionData->generatedID].regValNeeded)
					{ //TODO: we might need a varible like regValNeeded to tell us what register to read from Rt or Rd
					case instRegTypes::Rs:
						//Get the forwarded value
						instructionData->RsValHolder = forwardedValue;

						//Get remaining values the normal way
						instructionData->RtValHolder = sysCore.reg[instructionData->RtAddr];
						break;

					case instRegTypes::Rt:
						//Get the forwarded value
						instructionData->RtValHolder = forwardedValue;

						//Get remaining values the normal way
						instructionData->RsValHolder = sysCore.reg[instructionData->RsAddr];
						break;

					default:
						std::cerr << "\nERROR: [IDthread] given invalid forward register to read from\n\n";
						break;
					}

					break;

				default:
					std::cerr << "\nERROR: Format check bypassed [IDthread], Skipping instruction..\n" << std::endl;
					continue;
				}

				//Remove the item from the list
				sysCore.stageInfoID.useFwdHashTable.erase(instructionData->generatedID);
			}
			else
			{
				//Fetch the register values the normal way
				switch (instructionData->type)
				{
				case instFormat::Itype:
					instructionData->RsValHolder = sysCore.reg[instructionData->RsAddr];
					instructionData->RtValHolder = sysCore.reg[instructionData->RtAddr];
					instructionData->RdValHolder = sysCore.reg[instructionData->RdAddr];

					break;

				case instFormat::Rtype:
					instructionData->RsValHolder = sysCore.reg[instructionData->RsAddr];
					instructionData->RtValHolder = sysCore.reg[instructionData->RtAddr];

					break;

				default:
					std::cerr << "\nERROR: Format check bypassed [IDthread], Skipping instruction..\n" << std::endl;
					continue;
				}
			}

			//Pass instruction data to EX stage (will block if it cannot immediately acquire the lock)
			sysCore.IDtoEX.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
