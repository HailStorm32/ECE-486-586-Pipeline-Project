#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "ID_thread.h"
#include "decoder.h"

#define MIN_SLEEP_TIME		200  //In ms

void IDthread(SysCore& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::seconds delay(MIN_SLEEP_TIME);

	while (true)
	{
		//Check to see if we have a packet to work on
		if (!sysCore.IFtoID.empty())
		{
			//Get the instruction out of the queue
			uint32_t fullInstruction = sysCore.IFtoID.front();

			//Only coninue if the clock has changed and we have the go ahead from the master
			if (pastClkVal < sysCore.clk && sysCore.stageInfoID.okToRun) 
			{
				//Record the new clock value
				pastClkVal = sysCore.clk;

				//Since we can contiue, pop the instruction off the stack
				sysCore.IFtoID.pop();

				//Decode the instruction
				instInfoPtr_t instructionData = decodeInstruction(fullInstruction);

				if (instructionData == NULL)
				{
					std::cerr << "\nERROR: Invalid instruction: 0x" << std::hex << fullInstruction << std::dec << ", Skipping...\n\n";
					//TODO: Report to master thread that we have an invalid instruction
					sysCore.stageInfoID.error = true;
					continue;
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
					continue;
				}

				//Pass instruction data to EX stage
				sysCore.IDtoEX.push(instructionData);
			}
		}

		std::this_thread::sleep_for(delay);
	}
}
