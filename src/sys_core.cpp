#include <cstring>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sstream> // added for write fcn
#include <iostream> // added for write fcn
#include "sys_core.h"
#include "decoder.h"

uint32_t SysCore::findDataMem() {
	uint32_t currentLineNum = 1;
	std::string lineData = "";
	instInfoPtr_t instInfo = NULL;

	//Open the file
	std::ifstream file(filePath);

	//Make sure the file opened
	if (!file.is_open()) {
		std::cerr << "\nERROR: Unable to open file: " << filePath << "\n\n";
		exit(1);
	}

	//Cycle through all the lines
	while (std::getline(file, lineData))
	{
		//Decode the instruction
		instInfo = decodeInstruction(static_cast<uint32_t>(std::stoll(lineData, nullptr, 16)));

		if (instInfo == NULL) {
			std::cerr << "\nWARN: Invalid instruction: 0x" << std::hex << static_cast<uint32_t>(stoi(lineData)) << std::dec
				<< " on line [" << currentLineNum << "], Skipping...\n\n";
			currentLineNum++;
			continue;
		}

		//Check if instruction is of HALT opcode
		if (instInfo->opcode == opcodes::HALT) {
			delete instInfo;
			return ++currentLineNum;
		}
		else {
			currentLineNum++;
		}
	}
	delete instInfo;
	//If we have gotten this far, we never found the HALT instruction
	return UINT32_MAX;
}

std::string SysCore::getLineFromLineNum(const uint32_t targetLineNum)
{
	std::string lineData = "";
	uint32_t adjustedTargetLine = UINT32_MAX;

	std::ifstream file(filePath);

	//Make sure the file opened
	if (!file.is_open()) {
		std::cerr << "\nERROR: Unable to open file: " << filePath << "\n\n";
		exit(1);
	}

	//NOTE: We will need to offest our target by -1 b/c (total line length * our target line number) will always put us at the end of that line
	//  Given a line length of 9 bytes, if we want to read line 2, then 9*2=18 which will put the pointer at the end of line 2, and when we do a 
	//  getline, it will read the next line (3). Hence why we offset by -1. So with the offset, it will be 9*1=9 putting us at the end of line 1, 
	//  and the getline will read line 2, what we want
	adjustedTargetLine = (targetLineNum - 1);

	//Return if we are given an invalid line number
	if (adjustedTargetLine > totalNumOfLines)
	{
		std::cerr << "\nERROR: Given invalid line number [" << targetLineNum << "], max # of lines in file is: " << totalNumOfLines << "\n\n";
		return lineData;
	}

	//Move the file pointer to the target line
	file.seekg(totalFileLineLegnth * adjustedTargetLine, std::ios_base::beg);

	//Get the next line
	std::getline(file, lineData);

	file.close();

	return lineData;
}

uint32_t SysCore::getFileSize(const std::string filePath)
{
	uint32_t totalNumOfLines = 0;

	//Open the file
	std::ifstream file(filePath);

	//Make sure the file opened
	if (!file.is_open()) {
		std::cerr << "\nERROR: Unable to open file: " << filePath << "\n\n";
		return UINT32_MAX;
	}

	//Move the "get" pointer to the end of the file
	file.seekg(0, std::ios_base::end);

	//Get the current position of the "get" pointer, which is the size of the file in bytes
	std::streamsize fileSize = file.tellg();

	//Get the total number of lines the file has
	totalNumOfLines = fileSize / totalFileLineLegnth;

	file.close();

	return totalNumOfLines;
}

uint32_t SysCore::addrToLine(const uint32_t address)
{
	//Each line is 4 simulated bytes long. We add one b/c we index the lines from 1. 
	//  Integer division already floors the result, so we dont have to worry about that
	return (address / 4) + 1;
}

void SysCore::initDataMemTable()
{
	std::string lineData = "";
	uint32_t adjustedTargetLine = UINT32_MAX;
	const uint32_t TARGET_LINE = dataMemStartLine;
	uint32_t currentLineNum = dataMemStartLine;

	//Open the file
	std::ifstream file(filePath);

	//Make sure the file opened
	if (!file.is_open()) {
		std::cerr << "\nERROR: Unable to open file: " << filePath << "\n\n";
		exit(1);
	}

	//NOTE: We will need to offest our target by -1 b/c (total line length * our target line number) will always put us at the end of that line
	//  Given a line length of 9 bytes, if we want to read line 2, then 9*2=18 which will put the pointer at the end of line 2, and when we do a 
	//  getline, it will read the next line (3). Hence why we offset by -1. So with the offset, it will be 9*1=9 putting us at the end of line 1, 
	//  and the getline will read line 2, what we want
	adjustedTargetLine = (TARGET_LINE - 1);

	//Return if we are given an invalid line number
	if (adjustedTargetLine > totalNumOfLines)
	{
		std::cerr << "\nERROR: Given invalid line number [" << TARGET_LINE << "], max # of lines in file is: " << totalNumOfLines << "\n\n";
		exit(1);
	}

	//Move the file pointer to the target line
	file.seekg(totalFileLineLegnth * adjustedTargetLine, std::ios_base::beg);

	//Cycle through all the lines and write to the memory hash table
	while (std::getline(file, lineData))
	{
		//Create new memory cell
		memWordCellPtr_t memCell = new memWordCell_t;

		//Initalize cell
		memCell->value = static_cast<uint32_t>(std::stoll(lineData, nullptr, 16));
		memCell->hasBeenAccessed = false;
		
		//Match cell with line number and put into hash table
		dataMemoryHT.insert(std::make_pair(currentLineNum, memCell));

		currentLineNum++;
	}

	file.close();
}

void SysCore::writeDataMem(const uint32_t lineNum, const uint32_t data)
{
	memWordCellPtr_t memCell;
	
	//Lock hash table (gets unlocked as soon writeDataMem returns)
	std::unique_lock<std::mutex> lock(m_dataMemLock);

	//Get the memory cell
	memCell = dataMemoryHT[lineNum];

	//Write the new value
	memCell->value = data;

	//Log that the cell has been accessed
	memCell->hasBeenAccessed = true;
}

uint32_t SysCore::readDataMem(const uint32_t lineNum)
{
	memWordCellPtr_t memCell;

	//Lock hash table (gets unlocked as soon readDataMem returns)
	std::unique_lock<std::mutex> lock(m_dataMemLock);

	//Get the memory cell
	memCell = dataMemoryHT[lineNum];

	return memCell->value;
}

uint8_t SysCore::countLineEndings(const std::string& filePath)
{
	std::ifstream file(filePath, std::ios::binary);
	if (!file) {
		throw std::runtime_error("Could not open file");
	}

	char ch;
	uint8_t lineEndCount = 0, maxLineEndCount = 0;

	while (file.get(ch)) {
		if (ch == '\n' || ch == '\r') {
			++lineEndCount;
		}
		else if (lineEndCount > 0) {
			if (lineEndCount > maxLineEndCount) {
				maxLineEndCount = lineEndCount;
			}
			lineEndCount = 0;
		}
	}

	// Checking at the end in case the file ends with a line ending
	if (lineEndCount > maxLineEndCount) {
		maxLineEndCount = lineEndCount;
	}

	return maxLineEndCount;
}

void SysCore::printAccessedCells()
{
	for (const auto& pair : dataMemoryHT) {
		memWordCellPtr_t cell = pair.second;
		if (cell && cell->hasBeenAccessed) {
			std::cout << "Address: " << (pair.first - 1) * 4 << ", Value: " << cell->value << '\n';
		}
	}
}

// Core Constructor: Initialize variables and arrays to 0s
SysCore::SysCore(std::string filePath) {
	PC = 0;
	memset(reg, 0, sizeof(reg));
	clk = 0;
	memset(modifiedReg, 0, sizeof(modifiedReg));
	uint8_t lineEndingCount = 0;

	this->filePath = filePath;

	//Find what line endings the file uses
	lineEndingCount = countLineEndings(filePath);

	totalFileLineLegnth = FILE_LINE_LENGTH + lineEndingCount;

	instrCountStruct = {
		.arithmeticCount = 0,
		.logicalCount = 0,
		.memAccesCount = 0,
		.controlTransferCount = 0,
	};
	
	//Initalize stage structs
	stageInfoIF = {
		.stageType = fowardInfo::IF,
		.errorInfo = NULL,
		.errorType = errorCodes::ERR_NONE,
		.okToRun = false,
		.updatedPC =false,
		.invalidateData = false,
		.die = false,
		.fwdedAluResult = 0,
		.fwdedImmediate = 0,
	};
	stageInfoID = {
		.stageType = fowardInfo::ID,
		.errorInfo = NULL,
		.errorType = errorCodes::ERR_NONE,
		.okToRun = false,
		.updatedPC =false,
		.invalidateData = false,
		.die = false,
		.fwdedAluResult = 0,
		.fwdedImmediate = 0,
	};
	stageInfoEX = {
		.stageType = fowardInfo::EX,
		.errorInfo = NULL,
		.errorType = errorCodes::ERR_NONE,
		.okToRun = false,
		.updatedPC =false,
		.invalidateData = false,
		.die = false,
		.fwdedAluResult = 0,
		.fwdedImmediate = 0,
	};
	stageInfoMEM = {
		.stageType = fowardInfo::MEM,
		.errorInfo = NULL,
		.errorType = errorCodes::ERR_NONE,
		.okToRun = false,
		.updatedPC =false,
		.invalidateData = false,
		.die = false,
		.fwdedAluResult = 0,
		.fwdedImmediate = 0,
	};
	stageInfoWB = {
		.stageType = fowardInfo::WB,
		.errorInfo = NULL,
		.errorType = errorCodes::ERR_NONE,
		.okToRun = false,
		.updatedPC =false,
		.invalidateData = false,
		.die = false,
		.fwdedAluResult = 0,
		.fwdedImmediate = 0,
	};

	//Initalize stall tracker
	stallsRemaining = {
		.isInStall = false,
		.stallsRemainIF = 0,
		.stallsRemainID = 0,
		.stallsRemainEX = 0,
		.stallsRemainMEM = 0,
		.stallsRemainWB = 0,
	};


	//Get the total number of lines the file has
	if ((totalNumOfLines = getFileSize(filePath)) == UINT32_MAX) {
		std::cerr << "\nERROR: get_file_size returned UINT32_MAX" << "\n\n";
		exit(1);
	}

	//Find the start of the data memory
	if ((dataMemStartLine = findDataMem()) == UINT32_MAX) {
		std::cerr << "\nERROR: Unable to find start of data memory" << "\n\n";
		exit(1);
	}
	else {
		std::cout << "Found start of data memory on line: " << dataMemStartLine << std::endl;
	}

	//Initalize data memory hash table
	initDataMemTable();
}

// Memory Read method: 
uint32_t SysCore::memRead(const uint32_t address, const bool isInstMem) {
	uint32_t lineNumber = 0;
	std::string lineData = "";

	//Get line number
	lineNumber = addrToLine(address);

	if (isInstMem){
		//Warn if we are accessing outside of instuction memory space
		if (lineNumber >= dataMemStartLine) {
			std::cerr << "\nWARNING: Told to fetch instruction from data memory, proceeding... \n\n";
		}
		
		//Get line
		lineData = getLineFromLineNum(lineNumber);

		//Report error if we didnt get the full line
		if (lineData.size() != FILE_LINE_LENGTH) {
			std::cerr << "\nERROR: Recieved [" << lineData.size() << "] characters from line, "
				<< "expected " << FILE_LINE_LENGTH << "\n\n";
			return UINT32_MAX;
		}
		
		//Convert the string to uint32_t and return
		return static_cast<uint32_t>(std::stoll(lineData, nullptr, 16));
	}
	else{
		//If the line we are given is NOT in data memory
		if (lineNumber < dataMemStartLine)
		{
			std::cerr << "\nERROR: Address [0x" << std::hex << address << std::dec << "] converts to line ["
				<< lineNumber << "] and is outside of data memory\n\n";
			return UINT32_MAX;
		}

		//Get and return data
		return readDataMem(lineNumber);
	}
}

// Memory Write method:
uint32_t SysCore::memWrite(const uint32_t address, uint32_t value) {
	uint32_t lineNumber = 0;

	//Get line number
	lineNumber = addrToLine(address);

	if (lineNumber < dataMemStartLine)
	{
		std::cerr << "\nERROR: Address [0x" << std::hex << address << std::dec << "] converts to line ["
			<< lineNumber << "] and is outside of data memory\n\n";
		return UINT32_MAX;
	}

	writeDataMem(lineNumber, value);

    return 0;
}
