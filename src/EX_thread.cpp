#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "EX_thread.h"


#define MIN_SLEEP_TIME		200  //In ms

void EX_thread(SysCore& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::seconds delay(MIN_SLEEP_TIME);
	instInfoPtr_t instructionData;

	while (true)
	{		
		//Only coninue if the clock has changed and we have the go ahead from the master
		if (pastClkVal < sysCore.clk && sysCore.stageInfoID.okToRun)
		{
			//Try to get instruction out of the queue (will block if it cannot immediately acquire the lock)
			instructionData = sysCore.IDtoEX.pop();

			//If there was nothing for us to get, we missed our opportunity for this clock. Reset
			if (instructionData == NULL)
			{
				std::cout << "DEBUG: [IDthread] Missed opportunity for this clock, will try again next clock" << std::endl;
				sysCore.stageInfoID.okToRun = false;
				continue;
			}

			//Record the new clock value
			pastClkVal = sysCore.clk;

            
		//alu logic to be added
			

			//Pass instruction data to EX stage (will block if it cannot immediately acquire the lock)
			sysCore.IDtoEX.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
