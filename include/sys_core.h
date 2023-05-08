#ifndef __SYS_CORE_H
#define __SYS_CORE_H

#include <iostream>
#include <queue>
#include <cstdint>

class Sys_Core {
private:


public:        
    // Program Counter
    uint32_t PC;

    // Array of registers 0-31
    uint32_t reg[31];

    // 5 stages of pipline
    int IF, ID, EX, MEM, WB;

    // Buffers in between the stages
    std::queue<int> IFtoID;
    std::queue<int> IDtoEX;
    std::queue<int> EXtoMEM;
    std::queue<int> MEMtoWB;

    // Memory Read and Write methods
    uint32_t mem_read();
    uint32_t mem_write();
};
#endif
