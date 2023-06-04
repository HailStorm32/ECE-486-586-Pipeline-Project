#include "masterHelpers.h"


std::list<stageThreadPtr_t>* checkForErrors(SysCore& sysCore)
{
	std::list<stageThreadPtr_t>* erroredStagesList = new std::list<stageThreadPtr_t>;

	if (sysCore.stageInfoIF.errorType != errorCodes::ERR_NONE)
	{
		erroredStagesList->push_back(&sysCore.stageInfoIF);
	}
	if (sysCore.stageInfoID.errorType != errorCodes::ERR_NONE)
	{
		erroredStagesList->push_back(&sysCore.stageInfoID);
	}

	return erroredStagesList;
}

void removeOlderTarget(uint32_t consumerPC, std::list<stallTargetPtr_t>& targetList)
{
	stallTargetPtr_t target = NULL;

	//Cycle through the list (do the increament of the iterator in the else, b/c we delete an item from the list)
	for (std::list<stallTargetPtr_t>::iterator it = targetList.begin(); it != targetList.end();)
	{
		target = *it;

		if (target->targetPC == consumerPC)
		{
			it = targetList.erase(it);
		}
		else
		{
			++it;
		}
	}
}

int processError(SysCore& sysCore, std::list<stageThreadPtr_t>* structList)
{
	stageThreadPtr_t stageStruct = NULL;
	hazardErrInfoPtr_t hazardInfo = NULL;

	//Cycle through each stage struct and get the error
	for (std::list<stageThreadPtr_t>::iterator it = structList->begin(); it != structList->end(); ++it)
	{
		stageStruct = *it;

		if (stageStruct == NULL)
		{
			std::cerr << "\n\nERROR: [processError] stageStruct is NULL\n\n" << std::endl;
			continue;
		}

		//Filter by error type
		switch (stageStruct->errorType)
		{
		case errorCodes::ERR_HALT:
			//Tell all stages to die on next clock
			sysCore.stageInfoIF.die = true;
			sysCore.stageInfoID.die = true;
			sysCore.stageInfoEX.die = true;
			sysCore.stageInfoMEM.die = true;
			sysCore.stageInfoWB.die = true;

			//Remove the list b/c we are done with it
			delete structList;

			return 1;

		case errorCodes::ERR_INVALID_INST:
			//Tell the EX stage to invalidate the instruction
			//	We only will find if its an invalid instruction
			//	at the end of the ID stage. If we invalidate
			//	the instruction in the EX stage, EX wont pass instData
			//	to the further instructions causing them to stall
			//	which is what we want
			sysCore.stageInfoEX.invalidateData = true;
			break;

		case errorCodes::ERR_NOP:
			//Will most likely not be used, but its here just in case
			break;

		case errorCodes::ERR_BRANCH_TAKEN:
			//TODO: figure out what actualy needs to be invalidated
			
			//Flush the pipeline
			sysCore.stageInfoID.invalidateData = true;
			sysCore.stageInfoEX.invalidateData = true;
			sysCore.stageInfoMEM.invalidateData = true;
			sysCore.stageInfoWB.invalidateData = true;

			//TODO: clear all the error structs, stall counts, forward requests

			break;

		case errorCodes::ERR_RAW_HAZ:
			//Get the info on the hazard
			hazardInfo = static_cast<hazardErrInfoPtr_t>(stageStruct->errorInfo);

			//Create a tracker for stalls if we need it
			if (hazardInfo->numOfRequiredStalls > 0)
			{
				stallTargetPtr_t stallTarget = new stallTarget_t;

				if (stallTarget == NULL)
				{
					std::cerr << "\n\nERROR: [processError] faileed to create stallTarget\n\n" << std::endl;
					continue;
				}

				//If a stall target exists already for consumer instruction, remove it for the new one
				removeOlderTarget(hazardInfo->consumerExpectedPC, sysCore.stallTargetList);

				stallTarget->requiredStalls = hazardInfo->numOfRequiredStalls;
				stallTarget->targetPC = hazardInfo->consumerExpectedPC;

				//Add target to stallTargetList
				sysCore.stallTargetList.push_back(stallTarget);
			}

			//Figure out what stage we need to forward from
			if (hazardInfo->producerInstOpCode < opcodes::LDW) { //if an arithmetic or logical instruction
				//We forward from the EX stage

				//Create an entry into the EX stage struct to tell it that it will need to forward
				sysCore.stageInfoEX.useFwdHashTable[hazardInfo->producerInstID].fwdTo = fowardInfo::ID;
				sysCore.stageInfoEX.useFwdHashTable[hazardInfo->producerInstID].fwdedFrom = fowardInfo::NONE;

				//Create an entry into the ID stage struct to tell it that it will need to look for forwarded values
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdTo = fowardInfo::NONE;
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdedFrom = fowardInfo::EX;
			}
			else if (hazardInfo->producerInstOpCode > opcodes::XORI && hazardInfo->producerInstOpCode < opcodes::BZ) { //if memory instruction
				//We forward from the MEM stage

				//Create an entry into the MEM stage struct to tell it that it will need to forward
				sysCore.stageInfoMEM.useFwdHashTable[hazardInfo->producerInstID].fwdTo = fowardInfo::ID;
				sysCore.stageInfoMEM.useFwdHashTable[hazardInfo->producerInstID].fwdedFrom = fowardInfo::NONE;

				//Create an entry into the ID stage struct to tell it that it will need to look for forwarded values
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdTo = fowardInfo::NONE;
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdedFrom = fowardInfo::MEM;
			}

			//Freeup hazardInfo as we no longer need it
			delete hazardInfo;

			break;

		default:
			std::cerr << "\n\nWARNING: [processError] Control instruction is a producer, ignoring..\n\n" << std::endl;
			break;
		}

		//Clear the error state
		stageStruct->errorType = errorCodes::ERR_NONE;
	}
	
	//Remove the list b/c we are done with it
	delete structList;

	return 0;
}

void displayResults (SysCore& sysCore)
{
	std::cout << "\nInstruction Counts: " << std::endl;
	std::cout << "Arithmetic Instructions: " << sysCore.instrCountStruct.arithmeticCount << std::endl;
	std::cout << "Logical Instructions: " << sysCore.instrCountStruct.logicalCount << std::endl;
	std::cout << "Memory Access Instructions: " << sysCore.instrCountStruct.memAccesCount << std::endl;
	std::cout << "Control Transfer Instructions: " << sysCore.instrCountStruct.controlTransferCount << std::endl;
	std::cout << "\nProgram Counter: " << sysCore.PC << std::endl;
	std::cout << "\nFinal Register State:" << std::endl;
	for (int i = 0; i <32; i++){
		if (sysCore.modifiedReg[i] == 1) {
			std::cout << "R" << i << " : " << sysCore.reg[i]  << std::endl;
		}
	}
}

void applyStalls(SysCore& sysCore)
{
	stallTargetPtr_t target = NULL;

	//Cycle through the targets in the list (do the increament of the iterator in the else, b/c we delete an item from the list)
	for (std::list<stallTargetPtr_t>::iterator it = sysCore.stallTargetList.begin(); it != sysCore.stallTargetList.end();)
	{
		target = *it;

		//See if the target is for the current PC value 
		if (target->targetPC == sysCore.PC)
		{
			//Update the stall counter
			sysCore.stallsRemaining.stallsRemainIF = target->requiredStalls;

			//Free up the space used by the target
			delete target;

			//Remove the list item from the list 
			//Also assign the returned interator back to the loop iterator so that we can continue to loop
			it = sysCore.stallTargetList.erase(it);
		}
		else
		{
			++it;
		}
	}
}
