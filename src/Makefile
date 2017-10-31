CXX = g++
FLAGS = --std=c++0x -g -L../libs -I../include -Wno-deprecated-declarations
LIBS = -static -ljsoncpp
SOURCE_FILES = players.cpp main.cpp JSONUtils.cpp
BIN_FILE = ../game.exe

all:
	$(CXX) $(FLAGS) -o $(BIN_FILE) $(SOURCE_FILES) $(LIBS)