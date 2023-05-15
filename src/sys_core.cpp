#include <cstring>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cmath>
#include "sys_core.h"
#include "decoder.h"

uint32_t Sys_Core::find_data_mem(){
    uint32_t current_line_num = 1;
    std::string line_data = "";
    instInfoPtr_t inst_info = NULL;
   
    //Open the file
    std::ifstream file(file_path);

    //Make sure the file opened
    if (!file.is_open()) {
        std::cerr << "\nERROR: Unable to open file: " << file_path << "\n\n";
        exit(1);
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
            delete inst_info;
            return ++current_line_num;
        }
        else {
            current_line_num++;
        }
    }
    delete inst_info;
    //If we have gotten this far, we never found the HALT instruction
    return UINT32_MAX;
}

std::string Sys_Core::get_line_from_line_num(const uint32_t target_line_num)
{
    std::string line_data = "";
    uint32_t adjusted_target_line = UINT32_MAX;

    std::ifstream file(file_path);
   
    //Make sure the file opened
    if (!file.is_open()) {
        std::cerr << "\nERROR: Unable to open file: " << file_path << "\n\n";
        exit(1);
    }

    //NOTE: We will need to offest our target by -1 b/c (total line length * our target line number) will always put us at the end of that line
    //  Given a line length of 9 bytes, if we want to read line 2, then 9*2=18 which will put the pointer at the end of line 2, and when we do a 
    //  getline, it will read the next line (3). Hence why we offset by -1. So with the offset, it will be 9*1=9 putting us at the end of line 1, 
    //  and the getline will read line 2, what we want
    adjusted_target_line = (target_line_num - 1);

    //Return if we are given an invalid line number
    if (adjusted_target_line > total_num_of_lines)
    {
        std::cerr << "\nERROR: Given invalid line number [" << target_line_num << "], max # of lines in file is: " << total_num_of_lines << "\n\n";
        return line_data;
    }

    //Move the file pointer to the target line
    file.seekg(TOTAL_FILE_LINE_LENGTH * adjusted_target_line, std::ios_base::beg); 

    //Get the next line
    std::getline(file, line_data);

    file.close();

    return line_data;
}

uint32_t Sys_Core::get_file_size(const std::string file_path)
{
    uint32_t total_num_of_lines = 0;
    
    //Open the file
    std::ifstream file(file_path);

    //Make sure the file opened
    if (!file.is_open()) {
        std::cerr << "\nERROR: Unable to open file: " << file_path << "\n\n";
        return UINT32_MAX;
    }

    //Move the "get" pointer to the end of the file
    file.seekg(0, std::ios_base::end);

    //Get the current position of the "get" pointer, which is the size of the file in bytes
    std::streamsize file_size = file.tellg();

    //Get the total number of lines the file has
    total_num_of_lines = file_size / TOTAL_FILE_LINE_LENGTH;

    file.close();

    return total_num_of_lines;
}

uint32_t Sys_Core::addr_to_line(const uint32_t address)
{
    //Each line is 4 simulated bytes long. We add one b/c we index the lines from 1. 
    //  Integer division already floors the result, so we dont have to worry about that
    return (address / 4) + 1; 
}

// Core Constructor: Initialize variables and arrays to 0s
Sys_Core::Sys_Core(std::string file_path){
    PC = 0;
    memset(reg, 0, sizeof(reg));
    clk = 0;
 
    this->file_path = file_path;

    //Get the total number of lines the file has
    if ((total_num_of_lines = get_file_size(file_path)) == UINT32_MAX) {
        std::cerr << "\nERROR: get_file_size returned UINT32_MAX" << "\n\n";
        exit(1);
    }

    //Find the start of the data memory
    if ((data_mem_start_line = find_data_mem()) == UINT32_MAX){
        std::cerr << "\nERROR: Unable to find start of data memory" << "\n\n";
        exit(1);
    }
    else{
        std::cout << "Found start of data memory on line: " << data_mem_start_line << std::endl;
    }
}

// Memory Read method: 
uint32_t Sys_Core::mem_read(const uint32_t address, const bool is_inst_mem){
    uint32_t line_number = 0;
    std::string line_data = "";

    //If its an instruction memory access, we access the whole line
    if (is_inst_mem)
    {
        //Get line number
        line_number = addr_to_line(address);

        line_data = get_line_from_line_num(line_number);
    }


    return 0;
}

// Memory Write method:
uint32_t Sys_Core::mem_write(){
    return 0;
}
