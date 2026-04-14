CXX ?= c++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -O2

ROOT := ..
INCLUDES := -I$(ROOT)/include
SOURCES := $(ROOT)/source/util/nczblock.cpp test_nczblock.cpp
TARGET := test_nczblock

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
