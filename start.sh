#!/usr/bin/env bash

#########################
# Build and run project #
#########################

# Create directory for cmake files
mkdir -p build
cd build

# Remove existing executable
rm -rf bin

# Make the project
cmake -G "Unix Makefiles" ${PROJECT} # PROJECT variable set in dockerfile
make

echo $1

# Run the executable
if [[ $1 = "-d" ]]; then
    valgrind --leak-check=full -v bin/Reactant
else
    bin/Reactant
fi
