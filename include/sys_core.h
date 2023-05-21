#ifndef __SYS_CORE_H
#define __SYS_CORE_H

#include <cstdint>
#include <iostream>
#include "decoder.h"
#include "TSqueue.h"

#define FILE_LINE_LENGTH        8 //In bytes
#define FILE_LINE_END_LENGTH    1 //In bytes
#define TOTAL_FILE_LINE_LENGTH  (FILE_LINE_LENGTH + FILE_LINE_END_LENGTH)

// Enumerated values for telling a stage if they should read and/or write a forwarded value and to where
enum fowardInfo {
    NONE,
    TRUE,
    IF,
    ID,
    EX,
    MEM,
    WB
};


class SysCore {
private:

    // Stage struct to keep track of threads/stages
    typedef struct stage{
        // flags
        volatile fowardInfo fwdTo;
        volatile fowardInfo useFwdFrom;
        volatile bool error;
        volatile bool okToRun;
        volatile bool updatedPC; //true if EX found branch taken/jump, update PC with value in aluResult

        // forwarded vals
        volatile uint32_t Rs;
        volatile uint32_t Rt;
        volatile uint32_t Rd;
        volatile uint32_t aluResult;
        volatile uint16_t immediate;
    } stageThread_t;

    std::string filePath;
    uint32_t totalNumOfLines;

    /*
    * Description:
    *	Finds and returns the line number of the file
    *   for where data memory begins
    *
    * Arguments:
    *	None
    *
    * Return:
    *	uint32_t -- Line that marks the beginning of data memory
    *	UINT_MAX -- On error
    */
    uint32_t findDataMem();

    /*
    * Description:
    *	Returns given line from file
    *
    * Arguments:
    *	(INPUT) targetLineNum -- line of file (counts from 0)
    *
    * Return:
    *	string -- Line at given line number
    *   empty string -- If error
    */
    std::string getLineFromLineNum(const uint32_t targetLineNum);
    
    /*
    * Description:
    *	Returns the total number of lines the given file has
    *
    * Arguments:
    *	(INPUT) filePath -- path to the file
    *
    * Return:
    *	uint32_t -- total number of lines
    *   UINT_MAX -- On error
    */
    uint32_t getFileSize(const std::string filePath);

    /*
    * Description:
    *	Finds the line containing the specific address
    *
    * Arguments:
    *	(INPUT) address -- address location in memory file
    *
    * Return:
    *	uint32_t -- line that contains given address
    */
    uint32_t addrToLine(const uint32_t address);
    

public:        
    // Program Counter
    uint32_t PC;

    // Array of registers 0-31
    uint32_t reg[32];

    // Clock
    volatile long long clk;

    // 5 stages of pipline
    stageThread_t stageInfoIF;
    stageThread_t stageInfoID;
    stageThread_t stageInfoEX;
    stageThread_t stageInfoMEM;
    stageThread_t stageInfoWB;

    // Buffers in between the stages
    TSQueue<uint32_t> IFtoID;
    TSQueue<instInfoPtr_t> IDtoEX;
    TSQueue<instInfoPtr_t> EXtoMEM;
    TSQueue<instInfoPtr_t> MEMtoWB;

    // Contains line of file that starts data memory
    uint32_t dataMemStartLine;

    // Core Constructor
    SysCore(std::string filePath);

    /*
    * Description:
    *	Reads the value at a given address of the the memory file
    *       if the instrucion flag is true, then it will return
    *       the full line that the address belongs to and not just
    *       that address
    *
    * Arguments:
    *	(INPUT) address -- address location in memory file
    *   (INPUT) is_isnt_mem -- (for future compatibility boolean flag to indicate if reading from instruction memory
    *
    * Return:
    *	uint32_t -- data at give address, or full line (if an instrucion read)
    *   UINT_MAX -- On error
    */
    uint32_t memRead(const uint32_t address, const bool isInstMem);

    /*
    * Description:
    *	Writes the data to a given address of the memory file.
    *
    * Arguments:
    *	(INPUT) address -- address location in memory file
    *   (INPUT) value -- value to be written to the memory file
    *   (INPUT) isInstMem -- (for future compatibility) boolean flag to indicate if writing to instruction memory
    *
    * Return:
    *	None
    */
    // uint32_t memWrite(const uint32_t address, const uint32_t value, const bool isInstMem);
};
#endif
