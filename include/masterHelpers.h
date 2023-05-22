#pragma once
#include "sys_core.h"
#include <list>

/*
* Description:
*	Will check all the stage struct for any reported errors, and return 
*	where the errors occured in a TBD
*	
*
* Arguments:
*	 TBD will return info through an argument
*
* Return:
*	list<stageThreadPtr_t> -- pointer to list containing stage structs who have error codes to check
*	NULL -- no errors reported
*/
std::list<stageThreadPtr_t>* checkForErrors(SysCore& sysCore);