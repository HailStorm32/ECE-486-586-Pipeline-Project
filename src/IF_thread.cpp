#include <thread>
#include <chrono>
#include <cstdint>
#include <climits>
#include <iostream>
#include <stdlib.h>
#include "IF_thread.h"
#include "decoder.h"

#define MIN_SLEEP_TIME      50 // ms

bool findHazards(SysCore& sysCore, uint32_t rawInstruction);

void IFthread(SysCore& sysCore){
    long long prevClkVal = -1;
    std::chrono::milliseconds delay(MIN_SLEEP_TIME);
    
    uint32_t instruction;

    //Initalize random number gen
    srand(time(NULL));

    while (1){

        //See if we need to die or not
        if (sysCore.stageInfoIF.die) {
            return;
        }

        // Continue only if CLK has changed and go ahead from master thread
        if ((prevClkVal < sysCore.clk) && sysCore.stageInfoIF.okToRun) {
            // Clear our flag
            sysCore.stageInfoIF.okToRun = false;

            // Check and see if we have been given a new PC address from EX
            if (sysCore.stageInfoIF.updatedPC){
                //Reset the flag
                sysCore.stageInfoIF.updatedPC = false;

                //Set the new PC value
                sysCore.PC = sysCore.stageInfoIF.fwdedAluResult;
            }
            
            // read val at given address (PC val) from memory file
            instruction = sysCore.memRead(sysCore.PC, true);
            

            // Check if memRead() returns an error and apply flags
            if (instruction == UINT_MAX){
                std::cerr << "ERROR: [IFthread] SysCore::memRead() return on error" << std::endl;
                sysCore.stageInfoIF.okToRun = false;
                continue;
            }

            // Increment PC by 4 (bytes) aka 32 bits
            sysCore.PC = sysCore.PC + 4;

            // Record new CLK val 
            prevClkVal = sysCore.clk;

            //TODO: Run hazared checker

            instPreInfoPtr_t instPrePkg = new instPreInfo;

            //Generate a ID for the given instruction range form 0 - 0xFFFFFFFF
            instPrePkg->randID = rand() % 0xFFFFFFFF;
            //Write the instruction 
            instPrePkg->rawInstruction = instruction;

            // Push the returned instruction to buffer
            sysCore.IFtoID.push(instPrePkg);
        }

        // Apply delay
        std::this_thread::sleep_for(delay);
    }
}

bool findHazards(SysCore& sysCore, uint32_t rawInstruction)
{
    uint32_t producerPC = 0;
    instInfoPtr_t producerInstData = NULL;

    //Get the PC for the producer instruction
    producerPC = sysCore.PC - 4;

    //Decode the producer instruction
    producerInstData = decodeInstruction(rawInstruction);

    //Get the destination register the producer instruction uses
    switch (producerInstData->opcode)
    {
    case opcodes::ADD: case opcodes::SUB: case opcodes::MUL: case opcodes::OR: case opcodes::XOR:
    default:
        break;
    }

    

    return false;
}
