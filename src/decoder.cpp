#include "decoder.h"
#include <iostream>

instFormat returnInstType(uint8_t opcodeBits)
{
	switch (opcodeBits)
	{
	case opcodes::ADD: case opcodes::SUB: case opcodes::MUL: case opcodes::OR: case opcodes::AND: case opcodes::XOR:
		return instFormat::Rtype;

	case opcodes::ADDI: case opcodes::SUBI: case opcodes::MULI: case opcodes::ORI: case opcodes::ANDI: case opcodes::XORI: 
			case opcodes::LDW: case opcodes::STW: case opcodes::BZ: case opcodes::BEQ: case opcodes::JR: case opcodes::HALT:
		return instFormat::Itype;

	default:
		std::cout << "\nERROR: Invalid opcode given!\n" << std::endl;
		return instFormat::Invalid;
	}
}


instInfoPtr_t decodeInstruction(const uint32_t fullInstruction)
{
	uint8_t opcodeBits;
	instFormat format;

	//Obtain the opcode by shifting out the lower 26 bits
	opcodeBits = fullInstruction >> 26;
	
	//Get the format of the instruction (also acts as a check for the opcode)
	if ((format = returnInstType(opcodeBits)) == instFormat::Invalid)
	{
		//TODO: Return somthing other than NULL. Should trigger a hazard?
		return NULL;
	}

	//Allocate struct
	instInfoPtr_t instruction = new instInfo_t;

	//Set opcode and type
	instruction->opcode = static_cast<opcodes>(opcodeBits);
	instruction->type = format;

	switch (format)
	{
	case instFormat::Rtype:
		//Fill out and return struct

		//Get the Rs bits
		instruction->RsAddr = (fullInstruction >> 21) & 0x1F;

		//Get the Rt bits
		instruction->RtAddr = (fullInstruction >> 16) & 0x1F;

		//Get the Rd bits
		instruction->RdAddr = (fullInstruction >> 11) & 0x1F;

		return instruction;
	

	case instFormat::Itype:
		//Fill out and return struct

		//Get the Rs bits
		instruction->RsAddr = (fullInstruction >> 21) & 0x1F;

		//Get the Rt bits
		instruction->RtAddr = (fullInstruction >> 16) & 0x1F;

		//Get immediate bits
		instruction->immediateValHolder = ((fullInstruction & 0xFFFF) >> 2) | 0xC000;

		return instruction;


	default:
		std::cerr << "\nERROR: Invalid instruction format!\n" << std::endl;
		delete instruction;
		
		//TODO: Return somthing other than NULL. Should trigger a hazard?
		return NULL;
	}
}
//0010_0100_1010_0110_0000_0000_0000_0001   <-Value used for testing 24A60001
//001000 01010 00010 11010 00000000000  <- 2142D000
//001000 01010 00010 1101000001001100  <- 2D42D04C
