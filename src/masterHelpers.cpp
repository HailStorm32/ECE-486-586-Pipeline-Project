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
