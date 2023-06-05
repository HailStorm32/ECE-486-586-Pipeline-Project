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
	uint32_t fullInstruction;

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
			fullInstruction = sysCore.IFtoID.pop();

			//If there was nothing for us to get, we missed our opportunity for this clock. Reset
			if (fullInstruction == NULL)
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
			instInfoPtr_t instructionData = decodeInstruction(fullInstruction);

			if (instructionData == NULL)
			{
				std::cerr << "\nERROR: Invalid instruction: 0x" << std::hex << fullInstruction << std::dec << ", Skipping...\n\n";
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
				continue;
			}

			//Fetch the register values
			//TODO: Account for dependancy issues and forwarding
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

			//Pass instruction data to EX stage (will block if it cannot immediately acquire the lock)
			sysCore.IDtoEX.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
