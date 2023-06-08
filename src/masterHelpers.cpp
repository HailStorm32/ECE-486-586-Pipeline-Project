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
	if (sysCore.stageInfoEX.errorType != errorCodes::ERR_NONE)
	{
		erroredStagesList->push_back(&sysCore.stageInfoEX);
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
			
			sysCore.stageInfoIF.updatedPC = true;
			
			//Flush the pipeline
			sysCore.stageInfoID.invalidateData = true;
			sysCore.stageInfoEX.invalidateData = true;
			sysCore.stageInfoMEM.invalidateData = true;
			//sysCore.stageInfoWB.invalidateData = true;

			//Clear all the error structs
			//sysCore.stageInfoIF.errorType = errorCodes::ERR_NONE;
			//if (sysCore.stageInfoIF.errorInfo != NULL)
			//{
			//	delete sysCore.stageInfoIF.errorInfo;
			//	sysCore.stageInfoIF.errorInfo = NULL;
			//}
			//else
			//	sysCore.stageInfoIF.errorInfo = NULL;
			//sysCore.stageInfoEX.errorType = errorCodes::ERR_NONE;

			if (sysCore.stageInfoID.errorInfo != NULL)
			{
				delete sysCore.stageInfoID.errorInfo;
				sysCore.stageInfoID.errorInfo = NULL;
			}
			else
				sysCore.stageInfoID.errorInfo = NULL;

			if (sysCore.stageInfoEX.errorInfo != NULL)
			{
				delete sysCore.stageInfoEX.errorInfo;
				sysCore.stageInfoEX.errorInfo = NULL;
			}
			else
				sysCore.stageInfoEX.errorInfo = NULL;

			if (sysCore.stageInfoMEM.errorInfo != NULL)
			{
				delete sysCore.stageInfoMEM.errorInfo;
				sysCore.stageInfoMEM.errorInfo = NULL;
			}
			else
				sysCore.stageInfoMEM.errorInfo = NULL;

			//Clear the stall targets
			sysCore.stallTargetList.clear();
			sysCore.stallsRemaining.stallsRemainIF = 0;

			//Clear forwarding requests
			sysCore.stageInfoEX.useFwdHashTable.clear();
			sysCore.stageInfoMEM.useFwdHashTable.clear();
			sysCore.stageInfoID.useFwdHashTable.clear();


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
				sysCore.stageInfoEX.useFwdHashTable[hazardInfo->producerInstID].consumerInstID = hazardInfo->consumerInstID;

				//Create an entry into the ID stage struct to tell it that it will need to look for forwarded values
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdTo = fowardInfo::NONE;
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdedFrom = fowardInfo::EX;
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].regValNeeded = hazardInfo->consumerDependentReg;
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].consumerInstID = hazardInfo->consumerInstID;

				//Log what register we are forwarding
				if (isRdUsed(hazardInfo->producerInstOpCode))
				{
					sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdedRegister = instRegTypes::Rd;
				}
				else
				{
					sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdedRegister = instRegTypes::Rt;
				}

			}
			else if (hazardInfo->producerInstOpCode > opcodes::XORI && hazardInfo->producerInstOpCode < opcodes::BZ) { //if memory instruction
				//We forward from the MEM stage

				//Create an entry into the MEM stage struct to tell it that it will need to forward
				sysCore.stageInfoMEM.useFwdHashTable[hazardInfo->producerInstID].fwdTo = fowardInfo::ID;
				sysCore.stageInfoMEM.useFwdHashTable[hazardInfo->producerInstID].fwdedFrom = fowardInfo::NONE;
				sysCore.stageInfoMEM.useFwdHashTable[hazardInfo->producerInstID].consumerInstID = hazardInfo->consumerInstID;

				//Create an entry into the ID stage struct to tell it that it will need to look for forwarded values
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdTo = fowardInfo::NONE;
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdedFrom = fowardInfo::MEM;
				sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].regValNeeded = hazardInfo->consumerDependentReg;
				

				//Log what register we are forwarding
				if (isRdUsed(hazardInfo->producerInstOpCode))
				{
					sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdedRegister = instRegTypes::Rd;
				}
				else
				{
					sysCore.stageInfoID.useFwdHashTable[hazardInfo->consumerInstID].fwdedRegister = instRegTypes::Rt;
				}
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
	int totalInstructions = sysCore.instrCountStruct.arithmeticCount +
		sysCore.instrCountStruct.logicalCount +
		sysCore.instrCountStruct.memAccesCount +
		sysCore.instrCountStruct.controlTransferCount;

	std::cout << "\nInstruction Counts: " << totalInstructions << std::endl;
	std::cout << "Arithmetic Instructions: " << sysCore.instrCountStruct.arithmeticCount << std::endl;
	std::cout << "Logical Instructions: " << sysCore.instrCountStruct.logicalCount << std::endl;
	std::cout << "Memory Access Instructions: " << sysCore.instrCountStruct.memAccesCount << std::endl;
	std::cout << "Control Transfer Instructions: " << sysCore.instrCountStruct.controlTransferCount << std::endl;
	std::cout << "\nProgram Counter: " << sysCore.PC - 8 << std::endl;
	std::cout << "\nStalls: " << sysCore.instrCountStruct.stalls << std::endl;
	std::cout << "\nFinal Register States:" << std::endl;
	for (int i = 0; i <32; i++){
		if (sysCore.modifiedReg[i] == 1) {
			std::cout << "R" << i << " : " << static_cast<int>(sysCore.reg[i])  << std::endl;
		}
	}

	sysCore.printAccessedCells();
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

bool isRdUsed(uint32_t opCode)
{
	if (opCode == opcodes::ADD || opCode == opcodes::SUB || opCode == opcodes::AND || opCode == opcodes::OR || opCode == opcodes::XOR
		|| opCode == opcodes::MUL)
	{
		return true;
	}
	else
	{
		return false;
	}
}