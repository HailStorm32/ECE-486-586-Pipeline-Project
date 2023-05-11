#ifndef __SYS_CORE_H
#define __SYS_CORE_H

#include <queue>
#include <cstdint>

#include "decoder.h"

class Sys_Core {
private:

    // Stage struct to keep track of threads/stages
    typedef struct stage{
        // flags
        bool use_fwd;
        bool error;
        bool ok_run;

        // forwarded vals
        uint32_t Rs;
        uint32_t Rt;
        uint32_t Rd;
        uint16_t immediate;
    } stage_thread_t;

public:        
    // Program Counter
    uint32_t PC;

    // Array of registers 0-31
    uint32_t reg[31];

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

    // Core Constructor
    Sys_Core();

    // Memory Read and Write methods
    uint32_t mem_read();
    uint32_t mem_write();
};
#endif
