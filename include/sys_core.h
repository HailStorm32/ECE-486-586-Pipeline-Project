#ifndef __SYS_CORE_H
#define __SYS_CORE_H

#include <cstdint>
#include <iostream>
#include "decoder.h"
#include "TSqueue.h"
#include <thread>
#include <future>
#include <unordered_map>

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

enum errorCodes {
    ERR_NONE,
    ERR_HALT,
    ERR_NOP,
    ERR_INVALID_INST,
    ERR_BRANCH_TAKEN,
    ERR_RAW_HAZ,
};

// Struct to hold the details of a hazard error
typedef struct hazardErrInfo {
    uint32_t producerInstID;
    opcodes producerInstOpCode;
    
    uint32_t consumerInstID;
    opcodes consumerInstOpCode;
    uint32_t consumerExpectedPC;
    uint8_t numOfRequiredStalls;
}hazardErrInfo_t, *hazardErrInfoPtr_t;

// Goes into stallTargetList
typedef struct stallTarget {
    uint32_t targetPC;
    uint8_t requiredStalls;
}stallTarget_t, *stallTargetPtr_t;

// Goes into useFwdHashTable
typedef struct forwardDests {
    volatile fowardInfo fwdTo;
    volatile fowardInfo fwdFrom;
}forwardDests_t, * forwardDestsPtr_t;

// Stage struct to keep track of threads/stages
typedef struct stage {
    volatile fowardInfo stageType;

    // Hash table to hold instructions that a stage will need to use forworded values for
    std::unordered_map<uint32_t, forwardDests_t> useFwdHashTable;

    //Holds pointer to a struct that contains details regarding the reported error
    void* errorInfo;
    
    // flags
    volatile errorCodes errorType;
    volatile bool okToRun;
    volatile bool updatedPC; //true if EX found branch taken/jump, update PC with value in fwdedAluResult
    volatile bool invalidateData;
    volatile bool die;

    // forwarded vals
    volatile uint32_t fwdedRs;
    volatile uint32_t fwdedRt;
    volatile uint32_t fwdedRd;
    volatile uint32_t fwdedAluResult;
    volatile uint16_t fwdedImmediate;
} stageThread_t, * stageThreadPtr_t;

typedef struct count {
    int arithmeticCount;
    int logicalCount;
    int memAccesCount;
    int controlTransferCount;
} instrCount_t, * instrCountPtr_t;

class SysCore {
private:
    std::string filePath;
    uint32_t totalNumOfLines;

    // Struct that defines memory cell 
    typedef struct memWordCell {
        uint32_t value;
        bool hasBeenAccessed;
    }memWordCell_t, * memWordCellPtr_t;

    //Hash table that will contain all of data memory
    std::unordered_map<uint32_t, memWordCellPtr_t> dataMemoryHT;

    //Mutex for data memory
    std::mutex m_dataMemLock;

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
    *	Copies all all of data memory from the image file,
    *   and puts it in the dataMemoryHT
    *
    * Arguments:
    *	None
    *
    * Return:
    *	None
    */
    void initDataMemTable();

    /*
    * Description:
    *	Write data to the data memory hash table.
    *   Will update the memWordCell associated with the given
    *   line number and update hasBeenAccessed to true
    * 
    *   NOTE: This preforms no data validation, it is the   
    *       user's responsibility to preform this validation 
    *
    * Arguments:
    *	(INPUT) lineNum -- line number you want to write to
    *   (INPUT) data -- uint32_t data you want to write
    *
    * Return:
    *	None
    */
    void writeDataMem(const uint32_t lineNum, const uint32_t data);


    /*
    * Description:
    *	Will take the given line number and return the 
    *   data from the corresponding entry in the hash table
    *
    *   NOTE: This preforms no data validation, it is the
    *       user's responsibility to preform this validation
    *
    * Arguments:
    *	(INPUT) lineNum -- line number you want to write to
    *
    * Return:
    *	uint32_t -- data from the given line
    */
    uint32_t readDataMem(const uint32_t lineNum);
    

public:        
    // Program Counter
    uint32_t PC;

    // Array of registers 0-31
    uint32_t reg[32];

    //simple array to track if a register has been modified for final output results
    uint8_t modifiedReg[32];

   // instrCount_t instrCount;

    // Clock
    volatile long long clk;

    // 5 stages of pipline
    stageThread_t stageInfoIF;
    stageThread_t stageInfoID;
    stageThread_t stageInfoEX;
    stageThread_t stageInfoMEM;
    stageThread_t stageInfoWB;

    // Buffers in between the stages
    TSQueue<instPreInfoPtr_t> IFtoID;
    TSQueue<instInfoPtr_t> IDtoEX;
    TSQueue<instInfoPtr_t> EXtoMEM;
    TSQueue<instInfoPtr_t> MEMtoWB;

    // Contains line of file that starts data memory
    uint32_t dataMemStartLine;

    instrCount_t instrCountStruct;

    // Core Constructor
    SysCore(std::string filePath);

    struct stallTracker {
        bool isInStall;
        uint8_t stallsRemainIF;
        uint8_t stallsRemainID;
        uint8_t stallsRemainEX;
        uint8_t stallsRemainMEM;
        uint8_t stallsRemainWB;
    }stallsRemaining;

    std::list<stallTargetPtr_t> stallTargetList;

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
    uint32_t memWrite(const uint32_t address, const uint32_t value);
};
#endif
