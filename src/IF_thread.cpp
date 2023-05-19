#include <thread>
#include <chrono>
#include <cstdint>
#include <climits>
#include <iostream>
#include "IF_thread.h"

#define MIN_SLEEP_TIME      200 // ms

void IFthread(SysCore& sysCore){
    long long prevClkVal = -1;
    std::chrono::milliseconds delay(MIN_SLEEP_TIME);
    
    uint32_t instruction;

    while (1){
        // Continue only if CLK has changed and go ahead from master thread
        if ((prevClkVal < sysCore.clk) && sysCore.stageInfoIF.okToRun) {
            // read val at given address (PC val) from memory file
            instruction = sysCore.memRead(sysCore.PC, true);

            // Check if memRead() returns an error and apply flags
            if (instruction == UINT_MAX){
                std::cout << "ERROR: [IFthread] SysCore::memRead() return on error" << std::endl;
                sysCore.stageInfoIF.okToRun = false;
                continue;
            }

            // Increment PC by 4 (bytes) aka 32 bits
            sysCore.PC = sysCore.PC + 4;

            // Record new CLK val 
            prevClkVal = sysCore.clk;

            // Push the returned instruction to buffer
            sysCore.IFtoID.push(instruction);
        }

        // Apply delay of 200 ms
        std::this_thread::sleep_for(delay);
    }
}
