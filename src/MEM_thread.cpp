#include <thread>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <climits>
#include "MEM_thread.h"


#define MIN_SLEEP_TIME		50  //In ms
#define _VERBOSE_ 0

void MEMthread(SysCore& sysCore)
{
	long long pastClkVal = -1;
	std::chrono::milliseconds delay(MIN_SLEEP_TIME);
	instInfoPtr_t instructionData;
	bool forwardData = false;


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
				std::cout << "DEBUG: [MEMthread] No instruction recieved this clock, will try again next clock" << std::endl;
				continue;
			}

			//Skip the data if its invalid (aka we are told to flush)
			if (sysCore.stageInfoMEM.invalidateData)
			{
				std::cout << "DEBUG: [MEMthread] Skipped data due to invalid data this clock, will try again next clock" << std::endl;
				sysCore.stageInfoMEM.invalidateData = false;
				continue;
			}

			//Record the new clock value
			pastClkVal = sysCore.clk;


			//See if there is a forward request for the current instruction
			if (sysCore.stageInfoMEM.useFwdHashTable.count(instructionData->generatedID)) {

				if (sysCore.stageInfoMEM.useFwdHashTable[instructionData->generatedID].fwdedFrom != fowardInfo::NONE) {
					//TODO: add code to revieve forwarded values here
				}

				if (sysCore.stageInfoMEM.useFwdHashTable[instructionData->generatedID].fwdTo != fowardInfo::NONE) {
					forwardData = true;
				}
			}


			/*-------------------MEM Functionality Work in Progress-----------------------
			LDW Rt Rs Imm (Add the contents of Rs and the immediate value “Imm” to generate
			the effective address “A”, load the contents (32-bits) of the memory location at address
			“A” into register Rt).

			STW Rt Rs Imm (Add the contents of Rs and the immediate value “Imm” to generate
			the effective address “A”, store the contents of register Rt (32-bits) at the memory
			address “A”)
			*/



			//read data from memAddressValHolder calculated by EX stage and store in RtValholder    
			if (instructionData->opcode == LDW) {
				instructionData->RtValHolder = sysCore.memRead(instructionData->memAddressValHolder, false);

				// Check if memRead() returns an error and apply flags
				if (instructionData->RtValHolder == UINT_MAX) {
					std::cout << "ERROR: [MEMthread] SysCore::memRead() return on error" << std::endl;
					sysCore.stageInfoMEM.okToRun = false;
					continue;
				}

				#if (_VERBOSE_ > 0)
				std::cout << "VERBOSE: [MEMthread] mem. addrss: " << instructionData->memAddressValHolder << " value from mem. address: " << instructionData->RtValHolder << '\n';
				#endif
			}
			//write data from rtValHolder to address calculated by EX stage
			else if (instructionData->opcode == STW) {
				if (sysCore.memWrite(instructionData->memAddressValHolder, instructionData->RtValHolder) == UINT32_MAX) {
					std::cerr << "ERROR: [MEMthread] memWrite() returns UINT32_MAX" << '\n';
				}
				#if (_VERBOSE_ > 0)
				std::cout << "VERBOSE: [MEMthread] mem. addrss: " << instructionData->memAddressValHolder << " value from mem. address: " << instructionData->RtValHolder << '\n';
				#endif

			}
			else {
				//do nothing for now?
			}

			//Forward data if needed
			if (forwardData) {
				switch (sysCore.stageInfoMEM.useFwdHashTable[instructionData->generatedID].fwdTo) {

				case fowardInfo::IF:
					std::cerr << "\nWARNING: [MEMthread] Told to forward to IF when code doesnt exist, skipping..\n";
					break;

				case fowardInfo::ID:
					//Forward the data to the ID stage
					sysCore.stageInfoID.fwdedRt = instructionData->RtValHolder;
					break;

				case fowardInfo::EX:
					std::cerr << "\nWARNING: [MEMthread] Told to forward to EX when code doesnt exist, skipping..\n";
					break;

				case fowardInfo::MEM:
					std::cerr << "\nWARNING: [MEMthread] Told to forward to MEM when code doesnt exist, skipping..\n";
					break;

				case fowardInfo::WB:
					std::cerr << "\nWARNING: [MEMthread] Told to forward to WB when code doesnt exist, skipping..\n";
					break;

				default:
					std::cerr << "\nWARNING: [MEMthread] Given invalid destination to forward to, skipping..\n";
					break;
				}

				//Removed the forward request from the hash table
				sysCore.stageInfoMEM.useFwdHashTable.erase(instructionData->generatedID);

				//Reset the forward flag
				forwardData = false;
			}



			//Pass instruction data to WB stage (will block if it cannot immediately acquire the lock)
			sysCore.MEMtoWB.push(instructionData);
		}

		std::this_thread::sleep_for(delay);
	}
}
