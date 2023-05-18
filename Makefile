CC = g++
CPPFLAGS = -std=c++2a -Wall

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = .

# If any files are called 'all', 'default', 'test' or 'clean', make will ignore 
# and use the below targets instead
.PHONY = all default test clean

all: mips_lite

help:
	@echo "Targets: all, help mips_lite, clean"

mips_lite: $(SRC_DIR)/main.cpp $(OBJ_DIR)/sys_core.o $(OBJ_DIR)/decoder.o \
			$(OBJ_DIR)/ID_thread.o $(OBJ_DIR)/IF_thread.o
	$(CC) $(CPPFLAGS) -I $(INC_DIR) -o $@ $^

$(OBJ_DIR)/sys_core.o: $(SRC_DIR)/sys_core.cpp $(INC_DIR)/sys_core.h
	$(CC) $(CPPFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/decoder.o: $(SRC_DIR)/decoder.cpp $(INC_DIR)/decoder.h
	$(CC) $(CPPFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/IF_thread.o: $(SRC_DIR)/IF_thread.cpp $(INC_DIR)/IF_thread.h
	$(CC) $(CPPFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/ID_thread.o: $(SRC_DIR)/ID_thread.cpp $(INC_DIR)/ID_thread.h
	$(CC) $(CPPFLAGS) -I $(INC_DIR) -c $< -o $@

test:
	./mips_lite

clean:
	rm ./mips_lite $(OBJ_DIR)/*.o
