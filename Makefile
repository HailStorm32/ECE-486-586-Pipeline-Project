CC = g++
CPPFLAGS = -std=c++2a -Wall -g

SRC_DIR = ./src
INC_DIR = ./include

.PHONY = all default clean

all: mips_lite

help:
	@echo "Targets: all, mips_lite, sys_core.o, decoder.o, clean"

mips_lite: $(SRC_DIR)/main.cpp sys_core.o decoder.o
	$(CC) $(CPPFLAGS) -o $@ $^

sys_core.o: $(SRC_DIR)/sys_core.cpp $(INC_DIR)/sys_core.h
	$(CC) $(CPPFLAGS) -c $< -o $@

decoder.o: $(SRC_DIR)/decoder.cpp $(INC_DIR)/decoder.h
	$(CC) $(CPPFLAGS) -c $< -o $@

clean:
	rm ./mips_lite ./*.o
