#ifndef __SYS_CORE_H
#define __SYS_CORE_H

#include <iostream>
#include <cstdint>

class Sys_Core {
private:


public:        
    uint32_t PC;
    uint32_t reg[31];
    // TODO: Make 5 Variables for the 5 stages

    // TODO: Make 4 buffers or Queues

    uint32_t mem_read();
    uint32_t mem_write();
};
#endif
