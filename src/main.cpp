#include <iostream>
#include "../include/decoder.h"
#include "../include/sys_core.h"

int main(int argc, char *argv[])
{
    Sys_Core sys_core;

    std::cout << "Hello World!" << std::endl;

    decodeInstruction(0x24A60001);

	return 0;
}
