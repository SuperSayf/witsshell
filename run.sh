#!/bin/bash

# Clean, compile, and run witsshell.c using the Makefile

# Run the Makefile to clean and compile the program
make clean
make

mv witsshell src

cd src

# Check if compilation was successful
if [ $? -eq 0 ]; then
    # Run the compiled program
    ./witsshell
else
    echo "Compilation failed."
fi
