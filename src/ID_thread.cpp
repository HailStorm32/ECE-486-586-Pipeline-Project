#include "ID_thread.h"
#include "decoder.h"
#include <thread>
#include <chrono>
#include <cstdint>

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
			uint32_t fullInstruction = sysCore.IFtoID.front();

			//Only coninue if the clock has changed and we have the go ahead from the master
			if (1) //TODO: Check the core class clock and go ahead variable
			{
				//Since we can contiue, pop the instruction off the stack
				sysCore.IFtoID.pop();

				//Decode the instruction
				instInfoPtr_t instructionData = decodeInstruction(fullInstruction);

				if (instructionData == NULL)
				{
					//TODO: Report to master thread that we have an invalid instruction
					continue;
				}

			}
		}

		std::this_thread::sleep_for(delay);
	}
}
