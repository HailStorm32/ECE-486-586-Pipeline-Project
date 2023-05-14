#include <cstring>

#include "sys_core.h"

// Core Constructor: Initialize variables and arrays to 0s
Sys_Core::Sys_Core(){
    PC = 0;
    memset(reg, 0, sizeof(reg));
    clk = 0;
}

// Memory Read method: 
uint32_t Sys_Core::mem_read(){
    return 0;
}

// Memory Write method:
uint32_t Sys_Core::mem_write(){
    return 0;
}
