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


int main(int argc, char *argv[])
{
    // Initialize system core
    SysCore sysCore("given_sample_memory_image.txt");

    // Start threads (passing each thread a ref to the system core)
    std::thread ifThread(IFthread, std::ref(sysCore));
    std::thread idThread(IDthread, std::ref(sysCore));
    std::thread exThread(EXthread, std::ref(sysCore));
    std::thread memThread(MEMthread, std::ref(sysCore));

    //Start master loop
    while (true)
    {

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