CC = g++
CPPFLAGS = -std=c++2a -Wall -g
LDFLAGS = -pthread

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = .

OBJS = $(OBJ_DIR)/sys_core.o $(OBJ_DIR)/decoder.o $(OBJ_DIR)/ID_thread.o \
	  $(OBJ_DIR)/IF_thread.o $(OBJ_DIR)/EX_thread.o $(OBJ_DIR)/MEM_thread.o \
	  $(OBJ_DIR)/masterHelpers.o $(OBJ_DIR)/WB_thread.o

# If any files are called 'all', 'default', 'test' or 'clean', make will ignore 
# and use the below targets instead
.PHONY = all default test clean

all: mips_lite

help:
	@echo "Targets: all, help mips_lite, clean"

mips_lite: $(OBJS) 
	$(CC) $(CPPFLAGS) $(LDFLAGS) -I $(INC_DIR) -o $@ $(SRC_DIR)/main.cpp $^

$(OBJ_DIR)/masterHelpers.o: $(SRC_DIR)/masterHelpers.cpp $(INC_DIR)/masterHelpers.h
	$(CC) $(CPPFLAGS) $(LDFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/sys_core.o: $(SRC_DIR)/sys_core.cpp $(INC_DIR)/sys_core.h
	$(CC) $(CPPFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/decoder.o: $(SRC_DIR)/decoder.cpp $(INC_DIR)/decoder.h
	$(CC) $(CPPFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/IF_thread.o: $(SRC_DIR)/IF_thread.cpp $(INC_DIR)/IF_thread.h
	$(CC) $(CPPFLAGS) $(LDFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/ID_thread.o: $(SRC_DIR)/ID_thread.cpp $(INC_DIR)/ID_thread.h
	$(CC) $(CPPFLAGS) $(LDFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/EX_thread.o: $(SRC_DIR)/EX_thread.cpp $(INC_DIR)/EX_thread.h
	$(CC) $(CPPFLAGS) $(LDFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/MEM_thread.o: $(SRC_DIR)/MEM_thread.cpp $(INC_DIR)/MEM_thread.h
	$(CC) $(CPPFLAGS) $(LDFLAGS) -I $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/WB_thread.o: $(SRC_DIR)/WB_thread.cpp $(INC_DIR)/WB_thread.h
	$(CC) $(CPPFLAGS) $(LDFLAGS) -I $(INC_DIR) -c $< -o $@

test:
	./mips_lite

clean:
	rm ./mips_lite $(OBJ_DIR)/*.o
