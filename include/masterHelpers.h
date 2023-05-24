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
*	NULL -- no errors reported
*/
std::list<stageThreadPtr_t>* checkForErrors(SysCore& sysCore);

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
int actOnError(SysCore& sysCore, std::list<stageThreadPtr_t>& structList);