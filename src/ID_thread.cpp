#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "ID_thread.h"
#include "decoder.h"

#define MIN_SLEEP_TIME		200  //In ms

void IDthread(Sys_Core& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::seconds delay(MIN_SLEEP_TIME);

	while (true)
	{
		//Check to see if we have a packet to work on
		if (!sysCore.IFtoID.empty())
		{
			//Get the instruction out of the queue
			uint32_t fullInstruction = 0;//sysCore.IFtoID.front(); //TODO: Reenable when sys_core has been changed

			//Only coninue if the clock has changed and we have the go ahead from the master
			if (1) //TODO: Check the core class clock and go ahead variable
			{
				//Since we can contiue, pop the instruction off the stack
				sysCore.IFtoID.pop();

				//Decode the instruction
				instInfoPtr_t instructionData = decodeInstruction(fullInstruction);

				if (instructionData == NULL)
				{
					std::cout << "\nERROR: Invalid instruction: 0x" << std::hex << fullInstruction << std::dec << ", Skipping...\n\n";
					//TODO: Report to master thread that we have an invalid instruction
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
					std::cout << "\nERROR: Format check bypassed [IDthread], Skipping instruction..\n" << std::endl;
					continue;
				}

				//Pass instruction data to EX stage
				sysCore.IDtoEX.push(instructionData);

				//TODO: Take note of current clock value
			}
		}

		std::this_thread::sleep_for(delay);
	}
}
