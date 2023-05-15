#include <cstring>
#include <fstream>
#include "sys_core.h"

uint32_t Sys_Core::find_data_mem(){
    //Open the file
    std::ifstream file(file_path);

    //Make sure the file opened
    if (!file.is_open()){
        std::cerr << "\nERROR: unable to open file: " << file_path << std::endl;
    }
    return 0;
}

// Core Constructor: Initialize variables and arrays to 0s
Sys_Core::Sys_Core(std::string file_path){
    PC = 0;
    memset(reg, 0, sizeof(reg));
    clk = 0;

    //Find the start of the data memory
    if ((data_mem_start_line = find_data_mem()) == UINT32_MAX){
        std::cerr << "\nERROR: unable to find start of data memory" << std::endl;
    }
    else{
        std::cout << "Found start of data memory on line: " << data_mem_start_line << std::endl;
    }
}

// Memory Read method: 
uint32_t Sys_Core::mem_read(){
    return 0;
}

// Memory Write method:
uint32_t Sys_Core::mem_write(){
    return 0;
}
