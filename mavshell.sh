#!/bin/bash

gcc msh.c -o msh -std=c99
if [ $? -eq 0 ]; then
    ./msh
else
    echo "Compilation failed."
fi
