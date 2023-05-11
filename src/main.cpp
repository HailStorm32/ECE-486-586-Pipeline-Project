#include <iostream>
#include <chrono>   // researched that chrono header might be the choice 
                    // since it is portable to Windows and Linux

#include "../include/decoder.h"
#include "../include/sys_core.h"

int main(int argc, char *argv[])
{
    // Initialize system core
    Sys_Core sys_core;

    // Check what Sys_Core vars initialized to
    std::cout << "Sys_Core.PC = " << sys_core.PC << '\n'; 
    
    for (int i = 0; i < 32; i++){
        std::cout << "Sys_Core.reg = " << sys_core.reg[i] << '\n';
    }

    std::cout << "Sys_Core.clk = " << sys_core.clk << '\n';

    // Start threads (passing each thread a ref to the system core)


    //while (1){
        // if !stall:
        //  let all threads run
        //
        // if stall:
        //  determine which threads should run
        //  notify the threads can run and those that can't
        //  
        // sys_core.clk++;
        //
        // sleep for x time
    //}

	return 0;
}
