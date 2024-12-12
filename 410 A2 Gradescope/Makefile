# Define the compiler
CC = gcc

# Define the output file name
TARGET = myshell

# Define the source file
SRC = myshell.c

# Default target
all: $(TARGET)

# Rule for building the executable
$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET)

# Clean target for cleaning up the directory
clean:
	rm -f $(TARGET)
