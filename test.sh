#!/bin/bash

# Run tests for witsshell

make clean
make

mv witsshell src

cd src

./test-witsshell.sh