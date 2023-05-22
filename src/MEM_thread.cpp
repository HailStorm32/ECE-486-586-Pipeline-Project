#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "MEM_thread.h"


#define MIN_SLEEP_TIME		50  //In ms

void MEMthread(SysCore& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::milliseconds delay(MIN_SLEEP_TIME);
	instInfoPtr_t instructionData;

	//Only coninue if the clock has changed and we have the go ahead from the master
	if (pastClkVal < sysCore.clk && sysCore.stageInfoMEM.okToRun)
	{
		//Try to get instruction out of the queue (will block if it cannot immediately acquire the lock)
		instructionData = sysCore.EXtoMEM.pop();

		//If there was nothing for us to get, we missed our opportunity for this clock. Reset
		if (instructionData == NULL)
		{
			std::cout << "DEBUG: [MEMthread] Missed opportunity for this clock, will try again next clock" << std::endl;
			sysCore.stageInfoMEM.okToRun = false;
			return;
		}

		//Record the new clock value
		pastClkVal = sysCore.clk;

		/*-------------------MEM Functionality Work in Progress-----------------------
		When to write R-type ALU result to destination register for non-forwarding?
		EX stage, Here, or write back? Lecture slides seem to vary between MEM and WB.

		Store to memory here. store instruction complete?


		*/
		//Pass instruction data to WB stage (will block if it cannot immediately acquire the lock)
		sysCore.MEMtoWB.push(instructionData);
	}

}
