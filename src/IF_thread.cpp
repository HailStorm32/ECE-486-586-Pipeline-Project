#include <thread>
#include <chrono>
#include <cstdint>
#include <climits>
#include <iostream>
#include <stdlib.h>
#include "IF_thread.h"
#include "decoder.h"

#define HAZARD_SEARCH_DEPTH     3
#define MIN_SLEEP_TIME          50 // ms

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
    uint32_t tempPC = 0;
    instInfoPtr_t producerInstData = NULL;
    instInfoPtr_t consumerInstData = NULL;
    uint8_t producerDestReg = 0;
    uint8_t depthFound = 0;
    bool foundHazard = false;

    //Get the PC for the producer instruction
    tempPC = sysCore.PC - 4;

    //Decode the producer instruction
    producerInstData = decodeInstruction(rawInstruction);

    if (producerInstData == NULL)
    {
        //If we got an invalid instruction, let ID take care of it
        return false;
    }

    //Get the destination register the producer instruction uses
    if (producerInstData->RdAddr >= opcodes::BZ)
    {
        //If a control instruction is the producer, it will never cause a hazard
        return false;
    }
    if (producerInstData->RdAddr != UINT8_MAX) {
        //Instruction uses the Rd register for the destination
        producerDestReg = producerInstData->RdAddr;
    }
    else if (producerInstData->RdAddr == UINT8_MAX) {
        //Instruction uses the Rt register for the destination
        producerDestReg = producerInstData->RtAddr;
    }
    
    //Search the instructions further down to see if there is a data dependency 
    for (uint8_t index = 0; index < HAZARD_SEARCH_DEPTH; index++)
    {
        //Increment PC to the next instruction
        tempPC += 4;

        //Decode the instruction
        consumerInstData = decodeInstruction(tempPC);

        if (producerInstData == NULL)
        {
            delete producerInstData;

            //If we got an invalid instruction, let ID take care of it
            return false;
        }

        //Find dependency issues
        if (producerInstData->RdAddr != UINT8_MAX) {
            //Instruction uses the Rs and Rt registers for operands
            
            //Check to see if the producerDestReg matches any of the consumer operand registers
            if (producerInstData->RsAddr == producerDestReg || producerInstData->RtAddr == producerDestReg){
                foundHazard = true;
                depthFound = index + 1;

                //Break b/c by design only one stall set is needed. By the time the other dependant instructions get to run, the data will be available already
                break;
            }
        }
        else if (producerInstData->RdAddr == UINT8_MAX) {
            //Instruction uses the Rs register for operands
            
            //Check to see if the producerDestReg matches any of the consumer operand registers
            if (producerInstData->RsAddr == producerDestReg) {
                foundHazard = true;
                depthFound = index + 1;

                //Break b/c by design only one stall set is needed. By the time the other dependant instructions get to run, the data will be available already
                break;
            }
        }

        //Free up the space allocated for the decoded consumer instruction
        delete consumerInstData;
    }

    //Prepare an error report if we found a hazard
    if (foundHazard){
        //Create hazardErrInfo struct
        hazardErrInfoPtr_t hazardInfo = new hazardErrInfo_t;



    }

    return false;
}
