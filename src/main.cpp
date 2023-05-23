#include <iostream>
#include <thread>
#include <chrono>   // researched that chrono header might be the choice 
                    // since it is portable to Windows and Linux

#include "decoder.h"
#include "IF_thread.h"
#include "ID_thread.h"
#include "EX_thread.h"
#include "MEM_thread.h"
#include "sys_core.h"
#include "masterHelpers.h"

#define CLOCK_PERIOD    150 //in ms
#define SETTLE_TIME     1   //in ms

int main(int argc, char *argv[])
{
    // Initialize system core
    SysCore sysCore("given_sample_memory_image.txt");

    // Start threads (passing each thread a ref to the system core)
    std::thread ifThread(IFthread, std::ref(sysCore));
    std::thread idThread(IDthread, std::ref(sysCore));
    std::thread exThread(EXthread, std::ref(sysCore));
    std::thread memThread(MEMthread, std::ref(sysCore));

    std::chrono::milliseconds halfCycle(CLOCK_PERIOD/2);

    //Start master loop
    while (true)
    {
        std::cout << "DEBUG: [Masterthread] Clock: " << sysCore.clk << std::endl;

        //Check of reported errors
        if (checkForErrors(sysCore) != NULL)
        {
            //TODO: add code for processing errors
        }
        else
        {
            //No errors reported, continue as normal

            //Give the go ahead to all stages
            sysCore.stageInfoIF.okToRun = true;
            sysCore.stageInfoID.okToRun = true;
            sysCore.stageInfoEX.okToRun = true;
            sysCore.stageInfoMEM.okToRun = true;
            sysCore.stageInfoWB.okToRun = true;

            //Let threads finish first half of cycle
            std::this_thread::sleep_for(halfCycle/2);

            //Give the go ahead to all stages
            //sysCore.stageInfoIF.okToRun = true;
            sysCore.stageInfoID.okToRun = true;
            //sysCore.stageInfoEX.okToRun = true;
            //sysCore.stageInfoMEM.okToRun = true;
            sysCore.stageInfoWB.okToRun = true;

            //Increment the clock
            sysCore.clk++;

        }

        std::this_thread::sleep_for(halfCycle);
    }

    //while (1){
        // if !stall:
        //  let all threads run
        //
        // if stall:
        //  determine which threads should run
        //  notify the threads can run and those that can't
        //  
        // sysCore.clk++;
        //
        // sleep for x time
    //}

	return 0;
}