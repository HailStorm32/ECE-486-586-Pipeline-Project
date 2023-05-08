CC = g++
CPPFLAGS = -std=c++2a -Wall -g

# Directories
SRC_DIR = ./src
INC_DIR = ./include
OBJ_DIR = ./obj

# If any files are called 'all', 'default', or 'clean', make will ignore and
# use the below targets instead
.PHONY = all default clean

all: mips_lite

help:
	@echo "Targets: all, mips_lite, sys_core.o, decoder.o, clean"

mips_lite: $(SRC_DIR)/main.cpp $(OBJ_DIR)/sys_core.o $(OBJ_DIR)/decoder.o
	$(CC) $(CPPFLAGS) -o $@ $^

$(OBJ_DIR)/sys_core.o: $(SRC_DIR)/sys_core.cpp $(INC_DIR)/sys_core.h
	$(CC) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/decoder.o: $(SRC_DIR)/decoder.cpp $(INC_DIR)/decoder.h
	$(CC) $(CPPFLAGS) -c $< -o $@

clean:
	rm ./mips_lite $(OBJ_DIR)/*.o
