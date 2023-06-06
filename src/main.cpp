#include <iostream>
#include <thread>
#include <cstring>
#include <chrono>   // researched that chrono header might be the choice 
// since it is portable to Windows and Linux

#include "decoder.h"
#include "IF_thread.h"
#include "ID_thread.h"
#include "EX_thread.h"
#include "MEM_thread.h"
#include "WB_thread.h"
#include "sys_core.h"
#include "masterHelpers.h"

#define CLOCK_PERIOD    100 //in ms
#define SETTLE_TIME     1   //in ms

#define USE_TERMINAL	false //Set to false to be able to run program via a IDE and not terminal

int main(int argc, char* argv[])
{
	std::string filePath;

	if (USE_TERMINAL)
	{
		// Check if there were no arguements given
		if (argc < 2)
		{
			std::cerr << "Usage: ./mips_lite <path/to/file>" << std::endl;
			exit(EXIT_FAILURE);
		}

		// Check if too many arguments given
		if (argc > 2)
		{
			std::cerr << "ERROR: Too many arguments specified" << std::endl;
			exit(EXIT_FAILURE);
		}

		// store the filepath given to var
		filePath = argv[argc - 1];
	}
	else
	{
		std::cout << "\n\nUsing internal file path\n\n";

		//FOR DEBUG ONLY
		filePath = "hazardTest4.txt";
	}
	

	// Initialize system core
	SysCore sysCore(filePath);

	// Start threads (passing each thread a ref to the system core)
	std::thread ifThread(IFthread, std::ref(sysCore));
	std::thread idThread(IDthread, std::ref(sysCore));
	std::thread exThread(EXthread, std::ref(sysCore));
	std::thread memThread(MEMthread, std::ref(sysCore));
	std::thread wbThread(WBthread, std::ref(sysCore));

	std::chrono::milliseconds delay(CLOCK_PERIOD);

	std::list<stageThreadPtr_t>* errorsList = NULL;

	//Start master loop
	while (true)
	{
		std::cout << "DEBUG: [Masterthread] Clock: " << sysCore.clk << std::endl;

		errorsList = checkForErrors(sysCore);

		if (errorsList == NULL)
		{
			std::cerr << "\n\nERROR: [Masterthread] errorsList is NULL\n\n" << std::endl;
			continue;
		}

		//Check reported errors
		if (!errorsList->empty())
		{
			//Process the errors
			int ret = processError(sysCore, errorsList);

			//TODO: add code for processing errors

			//We had a HALT, so exit
			if (ret == 1)
			{
				displayResults(sysCore);
				exit(0);
			}
		}

		//Check and see if we need to apply any stalls
		applyStalls(sysCore);

		/*//Give the go ahead to all stages
		sysCore.stageInfoIF.okToRun = true;
		sysCore.stageInfoID.okToRun = true;
		sysCore.stageInfoEX.okToRun = true;
		sysCore.stageInfoMEM.okToRun = true;
		sysCore.stageInfoWB.okToRun = true;*/

		//Give the go ahead only to stages that are not stalled

		//IF stage
		if (sysCore.stallsRemaining.stallsRemainIF == 0)
		{
			sysCore.stageInfoIF.okToRun = true;
		}
		else
		{
			//Decrement the stall count
			sysCore.stallsRemaining.stallsRemainIF--;
		}

		//ID stage
		if (sysCore.stallsRemaining.stallsRemainID == 0)
		{
			sysCore.stageInfoID.okToRun = true;
		}
		else
		{
			//Decrement the stall count
			sysCore.stallsRemaining.stallsRemainID--;
		}

		//EX stage
		if (sysCore.stallsRemaining.stallsRemainEX == 0)
		{
			sysCore.stageInfoEX.okToRun = true;
		}
		else
		{
			//Decrement the stall count
			sysCore.stallsRemaining.stallsRemainEX--;
		}

		//MEM stage
		if (sysCore.stallsRemaining.stallsRemainMEM == 0)
		{
			sysCore.stageInfoMEM.okToRun = true;
		}
		else
		{
			//Decrement the stall count
			sysCore.stallsRemaining.stallsRemainMEM--;
		}

		//WB stage
		if (sysCore.stallsRemaining.stallsRemainWB == 0)
		{
			sysCore.stageInfoWB.okToRun = true;
		}
		else
		{
			//Decrement the stall count
			sysCore.stallsRemaining.stallsRemainWB--;
		}

		//Increment the clock
		sysCore.clk++;

		//delete errorsList;
		std::this_thread::sleep_for(delay);
	}

	//while (1){
		// if !stall:
		//  let all threads run
		//
		// if stall:
		//  determine which threads should run
		//  notify the threads can run and those that can't
		//  
		// sysCore.clk++;
		//
		// sleep for x time
	//}

	return 0;
}
