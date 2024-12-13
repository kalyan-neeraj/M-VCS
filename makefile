# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic  -I/usr/local/include
# OpenSSL libraries
LIBS = -lssl -lcrypto -L/usr/local/lib -lz

# Executable name
TARGET = ./mygit

# Source files
SRCS = main.cpp inputHandler.cpp command.cpp encoder.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Header
HDRS = $(wildcard *.h)

# Default rule
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# Rule to build object files
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET);
	rm -r ".mygit"

# Installation
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Phony targets
.PHONY: all clean install
