#include <cstring>
#include <fstream>
#include <string>
#include "sys_core.h"
#include "decoder.h"

uint32_t Sys_Core::find_data_mem(){
    uint32_t current_line_num = 0;
    std::string line_data = "";
    instInfoPtr_t inst_info = NULL;
   
    //Open the file
    std::ifstream file(file_path);

    //Make sure the file opened
    if (!file.is_open()){
        std::cerr << "\nERROR: Unable to open file: " << file_path << "\n\n";
        return UINT32_MAX;
    }

    //Cycle through all the lines
    while (std::getline(file, line_data))
    {
        //Decode the instruction
        inst_info = decodeInstruction(static_cast<uint32_t>(std::stoll(line_data, nullptr, 16)));
        
        if (inst_info == NULL){
            std::cerr << "\nWARN: Invalid instruction: 0x" << std::hex << static_cast<uint32_t>(stoi(line_data)) << std::dec 
                    << " on line [" << current_line_num << "], Skipping...\n\n";
            current_line_num++;
            continue;
        }

        //Check if instruction is of HALT opcode
        if (inst_info->opcode == opcodes::HALT){
            return ++current_line_num;
        }
        else {
            //std::cout << "Opcode found "
            current_line_num++;
        }
    }

    //If we have gotten this far, we never found the HALT instruction
    return UINT32_MAX;
}

std::string Sys_Core::get_line_from_line_num(uint32_t targ_line_num)
{
    std::ifstream file(file_path);
    uint32_t current_line_number = 0;
    std::string line = "";

    if (file.is_open()) {
        
        while (std::getline(file, line)) {
      
            if (current_line_number == targ_line_num) {
                break;
            }
            current_line_number++;
        }
        file.close();
    }
    else {
        std::cerr << "\nERROR: Unable to open file: " << file_path << "\n\n";
    }

    return line;
}

// Core Constructor: Initialize variables and arrays to 0s
Sys_Core::Sys_Core(std::string file_path){
    PC = 0;
    memset(reg, 0, sizeof(reg));
    clk = 0;

    this->file_path = file_path;

    //Find the start of the data memory
    if ((data_mem_start_line = find_data_mem()) == UINT32_MAX){
        std::cerr << "\nERROR: Unable to find start of data memory" << "\n\n";
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
