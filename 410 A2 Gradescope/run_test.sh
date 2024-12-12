#!/bin/bash

# Define the command
COMMAND="./tapper"

# Define the arguments
ARGS=(
    "-p1"
    "observe"
    "-p2"
    "reconstruct"
    "-p3"
    "tapplot"
    "-b"
    "async"
    "-s"
    "10"
    "arg1"
    "test_file_shm"
    "output_file_shm"
)

# Run the command with arguments
$COMMAND "${ARGS[@]}"
