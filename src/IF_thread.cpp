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

            // Record new CLK val 
            prevClkVal = sysCore.clk;

            //Run hazared checker
            findHazards(sysCore, instruction);

            instPreInfoPtr_t instPrePkg = new instPreInfo;

            //Generate a ID for the given instruction
            instPrePkg->generatedID = sysCore.PC + instruction;
            //Write the instruction 
            instPrePkg->rawInstruction = instruction;

            // Increment PC by 4 (bytes) aka 32 bits
            sysCore.PC = sysCore.PC + 4;

            // Push the returned instruction to buffer
            sysCore.IFtoID.push(instPrePkg);
        }

        // Apply delay
        std::this_thread::sleep_for(delay);
    }
}

bool findHazards(SysCore& sysCore, uint32_t rawProducerInstruction)
{
    uint32_t tempPC = 0;
    instInfoPtr_t producerInstData = NULL;
    instInfoPtr_t consumerInstData = NULL;
    uint32_t rawConsumerInstruction = 0;
    uint8_t producerDestReg = 0;
    uint8_t depthFound = 0;
    bool foundHazard = false;
    instRegTypes dependentRegister = instRegTypes::REG_NONE;

    //Get the PC for the producer instruction
    tempPC = sysCore.PC;

    //Return if we have wondered into data memory
    if (sysCore.addrToLine(tempPC) >= sysCore.dataMemStartLine)
    {
        return false;
    }

    //Decode the producer instruction
    producerInstData = decodeInstruction(rawProducerInstruction);

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

        //Return if we have wondered into data memory
        if (sysCore.addrToLine(tempPC) >= sysCore.dataMemStartLine)
        {
            delete producerInstData;
            return false;
        }

        //fetch the instruction
        rawConsumerInstruction = sysCore.memRead(tempPC, true);

        //Decode the instruction
        consumerInstData = decodeInstruction(rawConsumerInstruction);

        if (producerInstData == NULL)
        {
            delete producerInstData;

            //If we got an invalid instruction, let ID take care of it
            return false;
        }

        //Find dependency issues
        if (producerInstData->type == instFormat::Rtype) {
            //Instruction uses the Rs and Rt registers for operands
            
            //Check to see if the producerDestReg matches any of the consumer operand registers
            if (consumerInstData->RsAddr == producerDestReg){
                foundHazard = true;
                depthFound = index + 1;
                
                //Log what register needs the value
                dependentRegister = instRegTypes::Rs;

                //Break b/c by design only one stall set is needed. By the time the other dependant instructions get to run, the data will be available already
                break;
            }
            else if (consumerInstData->RtAddr == producerDestReg){
                foundHazard = true;
                depthFound = index + 1;

                //Log what register needs the value
                dependentRegister = instRegTypes::Rt;

                //Break b/c by design only one stall set is needed. By the time the other dependant instructions get to run, the data will be available already
                break;
            }
        }
        else if (producerInstData->type == instFormat::Itype) {
            //Instruction uses the Rs register for operands
            
            //Check to see if the producerDestReg matches any of the consumer operand registers
            if (consumerInstData->RsAddr == producerDestReg) {
                foundHazard = true;
                depthFound = index + 1;

                dependentRegister = instRegTypes::Rs;

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

        //Set the number of stalls based on when the consumer falls after the producer
        switch (depthFound)
        {
        case 1:
            hazardInfo->numOfRequiredStalls = 2;
            break;
        case 2:
            hazardInfo->numOfRequiredStalls = 1;
            break;
        case 3:
            hazardInfo->numOfRequiredStalls = 0;
            break;
        default:
            std::cerr << "\n\nERROR: [findHazards] invalid depth given \n";
            hazardInfo->numOfRequiredStalls = 0;
            break;
        }

        //Write the consumer instruction info
        hazardInfo->consumerExpectedPC = tempPC;
        hazardInfo->consumerInstID = tempPC + rawConsumerInstruction;
        hazardInfo->consumerInstOpCode = consumerInstData->opcode;
        hazardInfo->consumerDependentReg = dependentRegister;
        
        //Write producer instrucion info
        hazardInfo->producerInstID = (sysCore.PC) + rawProducerInstruction;
        hazardInfo->producerInstOpCode = producerInstData->opcode;

        //Log the error
        sysCore.stageInfoIF.errorType = errorCodes::ERR_RAW_HAZ;
        sysCore.stageInfoIF.errorInfo = hazardInfo;
    }

    //Free up resources
    delete producerInstData;
    if (foundHazard)
        delete consumerInstData;

    return false;
}
