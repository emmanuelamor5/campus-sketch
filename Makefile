# Simple Makefile alternative to CMake.
#   make        -> build ./campus
#   make run    -> build, then render assets/campus.scene into output/
#   make clean  -> remove build artifacts

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude
SRC      := $(wildcard src/*.cpp)
BIN      := campus

.PHONY: all run clean

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	mkdir -p output
	./$(BIN) assets/campus.scene output/campus.ppm output/campus_zoom.ppm

clean:
	rm -f $(BIN)
	rm -rf build
