#pragma once
#include "sys_core.h"
#include <list>

/*
* Description:
*	Will check all the stage struct for any reported errors, and return 
*	a list containing pointers to all the stage structs with errors
*	
*
* Arguments:
*	 sysCore -- Refernce to system Core class
*
* Return:
*	list<stageThreadPtr_t> -- pointer to list containing stage structs who have error codes to check
*	empty list -- no errors reported
*/
std::list<stageThreadPtr_t>* checkForErrors(SysCore& sysCore);

/*
* Description:
*	If it exists, remove older target from stallTargetList
*
*
* Arguments:
*	 consumerPC -- PC of instruction we are looking to remove
*	 targetList -- reference to list of targets
*
* Return:
*	NONE
*/
void removeOlderTarget(uint32_t consumerPC, std::list<stallTargetPtr_t>& targetList);

/*
* Description:
*	Will interpret the errors reported and act accordingly 
*
*
* Arguments:
*	 sysCore -- Reference to system Core class
*	 structList -- Reference to list containing pointers to all the stage structs with errors
*	 
*
* Return:
*	0 -- If no other action needs to be taken
*	1 -- If program needs to be stopped
*/
int processError(SysCore& sysCore, std::list<stageThreadPtr_t>* structList);

void displayResults (SysCore& sysCore);

/*
* Description:
*	Checks the stallTargetList for stall targets and will apply
*	stalls to the IF stage if the target meets the criteria to run
*
*
* Arguments:
*	 sysCore -- Reference to system Core class
* 
*
* Return:
*	None
*/
void applyStalls(SysCore& sysCore);