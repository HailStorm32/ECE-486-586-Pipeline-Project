#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <climits>
#include "MEM_thread.h"


#define MIN_SLEEP_TIME		50  //In ms

void MEMthread(SysCore& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::milliseconds delay(MIN_SLEEP_TIME);
	instInfoPtr_t instructionData;
    //uint32_t instruction;


	while (true)
	{		
		//See if we need to die or not
		if (sysCore.stageInfoMEM.die) {
			return;
		}

		//Only coninue if the clock has changed and we have the go ahead from the master
		if (pastClkVal < sysCore.clk && sysCore.stageInfoMEM.okToRun)
		{
			//Clear our flag
			sysCore.stageInfoMEM.okToRun = false;

			//Try to get instruction out of the queue (will block if it cannot immediately acquire the lock)
			instructionData = sysCore.EXtoMEM.pop();

			//If there was nothing for us to get, we missed our opportunity for this clock. Reset
			if (instructionData == NULL)
			{
				std::cout << "DEBUG: [MEMthread] Missed opportunity for this clock, will try again next clock" << std::endl;
				continue;
			}

			//Skip the data if its invalid (aka we are told to flush)
			if (sysCore.stageInfoMEM.invalidateData)
			{
				std::cout << "DEBUG: [MEMthread] Missed opportunity for this clock, will try again next clock" << std::endl;
				sysCore.stageInfoMEM.invalidateData = false;
				continue;
			}

            // Load (Read from memory using the aluResultHolder)
            //instruction = sysCore.memRead(instructionData->aluResultHolder, true);

            // Check if memRead() returns an error and apply flags
            //if (instruction == UINT_MAX){
            //    std::cout << "ERROR: [MEMthread] SysCore::memRead() return on error" << std::endl;
            //    sysCore.stageInfoMEM.okToRun = false;
            //    continue;
            //}

			//Record the new clock value
			pastClkVal = sysCore.clk;

			/*-------------------MEM Functionality Work in Progress-----------------------
            
            Read/write memory. I believe store instructions are 'complete' here.
			Load/R-type instructions will have actual register updated in WB stage
    
                   
			*/

            // LMD = Mem[ALUout]
			//Pass instruction data to WB stage (will block if it cannot immediately acquire the lock)
			sysCore.MEMtoWB.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
