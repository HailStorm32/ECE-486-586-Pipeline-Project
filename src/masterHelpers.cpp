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

int actOnError(SysCore& sysCore, std::list<stageThreadPtr_t>* structList)
{
	stageThreadPtr_t stageStruct = NULL;

	//Cycle through the errors
	for (std::list<stageThreadPtr_t>::iterator it = structList->begin(); it != structList->end(); ++it)
	{
		stageStruct = *it;
	}



	return 0;
}
