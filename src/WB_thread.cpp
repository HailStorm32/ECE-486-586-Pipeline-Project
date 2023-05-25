#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "WB_thread.h"

#define MIN_SLEEP_TIME      50 // ms

void WBthread(SysCore& sysCore)
{
    long long pastClkVal = -1;
	std::chrono::milliseconds delay(MIN_SLEEP_TIME);
	instInfoPtr_t instructionData;

    while (true)
        {		
            //See if we need to die or not
            if (sysCore.stageInfoWB.die) {
                return;
            }

            //Only coninue if the clock has changed and we have the go ahead from the master
            if (pastClkVal < sysCore.clk && sysCore.stageInfoWB.okToRun)
            {
                //Clear our flag
                sysCore.stageInfoWB.okToRun = false;

                //Try to get instruction out of the queue (will block if it cannot immediately acquire the lock)
                instructionData = sysCore.MEMtoWB.pop();

                //If there was nothing for us to get, we missed our opportunity for this clock. Reset
                if (instructionData == NULL)
                {
                    std::cout << "DEBUG: [WBthread] Missed opportunity for this clock, will try again next clock" << std::endl;
                    continue;
                }

                //Skip the data if its invalid (aka we are told to flush)
                if (sysCore.stageInfoWB.invalidateData)
                {
                    std::cout << "DEBUG: [WBthread] Missed opportunity for this clock, will try again next clock" << std::endl;
                    sysCore.stageInfoWB.invalidateData = false;
                    continue;
                }

                //Record the new clock value
                pastClkVal = sysCore.clk;

                /*-------------------Implement WB Functionality Work in Progress-----------------------*/
                
            }

            std::this_thread::sleep_for(delay);
        }



}