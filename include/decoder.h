#pragma once
#include <cstdint>

enum opcodes
{
	//Arithmetic
	ADD		= 0b000000, ///R-type
	ADDI	= 0b000001,
	SUB		= 0b000010, ///R-type
	SUBI	= 0b000011,
	MUL		= 0b000100, ///R-type
	MULI	= 0b000101,

	//Logical
	OR		= 0b000110, ///R-type
	ORI		= 0b000111,
	AND		= 0b001000, ///R-type
	ANDI	= 0b001001,
	XOR		= 0b001010, ///R-type
	XORI	= 0b001011,

	//Memory Access
	LDW		= 0b001100,
	STW		= 0b001101,

	//Control Flow
	BZ		= 0b001110,
	BEQ		= 0b001111,
	JR		= 0b010000,
	HALT	= 0b010001,
	
	//For code error catching
	INVALID = 0b111111
};

enum instFormat
{
	Rtype,
	Itype,
	Invalid
};
 
typedef struct instInfo
{
	opcodes opcode;
	instFormat type;
	uint8_t RsAddr;
	uint8_t RtAddr;
	uint8_t RdAddr;
	uint32_t RsVal;
	uint32_t RtVal;
	uint32_t RdVal;
	uint16_t immediate;
	uint32_t aluResult;
}instInfo_t, *instInfoPtr_t;

/*
* Description:
*	Decode a given integer instruction and break it down 
*	into its differnt parts. 
*
* Arguments:
*	(INPUT) fullInstruction -- 32bit instruction
*
* Return:
*	instInfoPtr_t -- Pointer to struct containing instuction parts
*	NULL -- If invalid instruction
*/
instInfoPtr_t decodeInstruction(const uint32_t fullInstruction);