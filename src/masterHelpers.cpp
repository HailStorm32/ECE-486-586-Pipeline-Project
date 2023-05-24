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

int processError(SysCore& sysCore, std::list<stageThreadPtr_t>* structList)
{
	stageThreadPtr_t stageStruct = NULL;

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
			return 1;

		case errorCodes::ERR_INVALID_INST:
			//Tell the EX stage to invalidate the data
			sysCore.stageInfoEX.invalidateData = true;
			break;

		case errorCodes::ERR_NOP:
			break;

		case errorCodes::ERR_RAW_HAZ:
			break;

		default:
			std::cerr << "\n\nWARNING: [processError] received invalid error\n\n" << std::endl;
			break;
		}

	}

	switch (stageStruct->stageType)
	{
	case fowardInfo::IF:
		break;

	case fowardInfo::ID:
		break;

	case fowardInfo::EX:
		break;

	case fowardInfo::MEM:
		break;

	case fowardInfo::WB:
		break;

	default:

		break;
	}


	return 0;
}
