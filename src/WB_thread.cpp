#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include "WB_thread.h"

#define MIN_SLEEP_TIME      50 // ms
#define _VERBOSE_ 0

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

                /*-------------------Implement WB Functionality Work in Progress-----------------*/
            
                //if Rtype instr update RD register
                if(instructionData->type == Rtype) {
                    sysCore.reg[instructionData->RdAddr] = instructionData->RdValHolder;
                    //mark this register as modified
                    sysCore.modifiedReg[instructionData->RdAddr] = 1;
                    #if (_VERBOSE_ > 0)
                    std::cout << "Writing Value: " << instructionData->RdValHolder << " to Reg: " << static_cast<int>(instructionData->RdAddr) << '\n';
                    std::cout << "syscore.reg Value: " << sysCore.reg[instructionData->RdAddr] << '\n';
                    #endif  
                }
                else if(instructionData->type == Itype) {
                    
                    //if its not a control flow or store op, update register Rt
                    if(!instructionData->isControlFlow && instructionData->opcode != STW){
                        sysCore.reg[instructionData->RtAddr] = instructionData->RtValHolder;
                        //mark this register as modified
                        sysCore.modifiedReg[instructionData->RtAddr] = 1;
                    } 
                    else {
                        //do nothing for now
                    }  
                    #if (_VERBOSE_ > 0)
                    std::cout << "Writing Value: " << instructionData->RtValHolder << " to Reg: " << static_cast<int>(instructionData->RtAddr) << '\n';
                    std::cout << "syscore.reg Value: " << sysCore.reg[instructionData->RtAddr] << '\n';
                    #endif           
                }
                else {
                     std::cerr << "DEBUG: [WBthread] unknown instruction type encountered" << std::endl;
                }  
            }

            std::this_thread::sleep_for(delay);
        }



}