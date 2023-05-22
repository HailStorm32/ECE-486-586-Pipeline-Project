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

	//Only coninue if the clock has changed and we have the go ahead from the master
	if (sysCore.stageInfoID.okToRun)
	{
		//Try to get instruction out of the queue (will block if it cannot immediately acquire the lock)
		fullInstruction = sysCore.IFtoID.pop();

		//If there was nothing for us to get, we missed our opportunity for this clock. Reset
		if (fullInstruction == NULL)
		{
			std::cout << "DEBUG: [IDthread] Missed opportunity for this clock, will try again next clock" << std::endl;
			sysCore.stageInfoID.okToRun = false;
			return;
		}

		//Record the new clock value
		pastClkVal = sysCore.clk;

		//Decode the instruction
		instInfoPtr_t instructionData = decodeInstruction(fullInstruction);

		if (instructionData == NULL)
		{
			std::cerr << "\nERROR: Invalid instruction: 0x" << std::hex << fullInstruction << std::dec << ", Skipping...\n\n";
			//TODO: Report to master thread that we have an invalid instruction
			sysCore.stageInfoID.error = true;
			return;
		}

		//Fetch the register values
		//TODO: Account for dependancy issues and forwarding
		switch (instructionData->type)
		{
		case instFormat::Itype:
			instructionData->RsVal = sysCore.reg[instructionData->RsAddr];
			instructionData->RtVal = sysCore.reg[instructionData->RtAddr];
			instructionData->RdVal = sysCore.reg[instructionData->RdAddr];

			break;

		case instFormat::Rtype:
			instructionData->RsVal = sysCore.reg[instructionData->RsAddr];
			instructionData->RtVal = sysCore.reg[instructionData->RtAddr];

			break;

		default:
			std::cerr << "\nERROR: Format check bypassed [IDthread], Skipping instruction..\n" << std::endl;
			return;
		}

		//Pass instruction data to EX stage (will block if it cannot immediately acquire the lock)
		sysCore.IDtoEX.push(instructionData);
	}
}
