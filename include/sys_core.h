#ifndef __SYS_CORE_H
#define __SYS_CORE_H

#include <queue>
#include <cstdint>
#include <iostream>
#include "decoder.h"

#define FILE_LINE_LENGTH        8 //In bytes
#define FILE_LINE_END_LENGTH    1 //In bytes
#define TOTAL_FILE_LINE_LENGTH  (FILE_LINE_LENGTH + FILE_LINE_END_LENGTH)

class Sys_Core {
private:

    // Stage struct to keep track of threads/stages
    typedef struct stage{
        // flags
        volatile bool use_fwd;
        volatile bool error;
        volatile bool ok_run;

        // forwarded vals
        volatile uint32_t Rs;
        volatile uint32_t Rt;
        volatile uint32_t Rd;
        volatile uint16_t immediate;
    } stage_thread_t;

    std::string file_path;
    uint32_t total_num_of_lines;

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
    uint32_t find_data_mem();

    /*
    * Description:
    *	Returns given line from file
    *
    * Arguments:
    *	(INPUT) target_line_num -- line of file (counts from 0)
    *
    * Return:
    *	string -- Line at given line number
    *   empty string -- If error
    */
    std::string get_line_from_line_num(const uint32_t target_line_num);
    
    /*
    * Description:
    *	Returns the total number of lines the given file has
    *
    * Arguments:
    *	(INPUT) file_path -- path to the file
    *
    * Return:
    *	uint32_t -- total number of lines
    *   UINT_MAX -- On error
    */
    uint32_t get_file_size(const std::string file_path);

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
    uint32_t addr_to_line(const uint32_t address);
    

public:        
    // Program Counter
    uint32_t PC;

    // Array of registers 0-31
    uint32_t reg[32];

    // Clock
    long long clk;

    // 5 stages of pipline
    stage_thread_t stageInfoIF;
    stage_thread_t stageInfoID;
    stage_thread_t stageInfoEX;
    stage_thread_t stageInfoMEM;
    stage_thread_t stageInfoWB;

    // Buffers in between the stages
    std::queue<uint32_t> IFtoID;
    std::queue<instInfoPtr_t> IDtoEX;
    std::queue<instInfoPtr_t> EXtoMEM;
    std::queue<instInfoPtr_t> MEMtoWB;

    // Contains line of file that starts data memory
    uint32_t data_mem_start_line;

    // Core Constructor
    Sys_Core(std::string file_path);

    // Memory Read and Write methods
    uint32_t mem_read(const uint32_t address, const bool is_inst_mem);
    uint32_t mem_write();
};
#endif
