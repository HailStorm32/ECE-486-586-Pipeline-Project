#include <iostream>
#include "../include/decoder.h"
#include "../include/sys_core.h"

int main(int argc, char *argv[])
{
    Sys_Core sys_core;

    std::cout << "Sys_Core.PC = " << sys_core.PC << '\n'; 
    
    for (int i = 0; i < 32; i++){
        std::cout << "Sys_Core.reg = " << sys_core.reg[i] << '\n';
    }

    std::cout << "Sys_Core.clk = " << sys_core.clk << '\n';

	return 0;
}
