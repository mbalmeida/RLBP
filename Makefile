# Declaration of variables
CC = g++
CCFLAGS = -w -Wall -O -std=c++11

# File names
EXEC = RLBP
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
LDFLAGS = -lboost_program_options -lopencv_highgui -lopencv_core

# Main target
$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) $(CCFLAGS) $(LDFLAGS) -o $(EXEC)

# To obtain object files
%.o: %.cpp
	$(CC) -c $(CCFLAGS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS)
