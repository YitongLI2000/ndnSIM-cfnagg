#!/bin/bash

# Clear all previous log files
GRAPH_DIR="./src/ndnSIM/results/graphs"
LOGS_DIR="./src/ndnSIM/results/logs"

# Define function to remove files from specific path
remove_files() {
    local directory=$1
    local extension=$2

    # Check if directory exists
    if [ -d "$directory" ]; then
        # Remove all files with specific extension
        echo "Removing all *.$extension files in $directory"
        rm -f $directory/*.$extension

        # Check if the removal was successful
        if [ $? -eq 0 ]; then
            echo "All *.$extension files have been removed successfully from $directory."
        else
            echo "Failed to remove *.extension files from $directory."
        fi
    else
        echo "Directory does not exist: $directory, no need for further operation."
    fi
}

# Remove .txt and .png from specific folder
remove_files "$GRAPH_DIR" "png"
remove_files "$LOGS_DIR" "txt"




# Check if a Python version argument was provided
if [ -z "$1" ]; then
  echo "Usage: $0 <python_version>"
  exit 1
fi

# Use the provided argument to find the Python executable path
PYTHON=$(which "$1")

# Check if the Python executable was found
if [ -z "$PYTHON" ]; then
  echo "Error: Python executable not found for version $1"
  exit 1
fi

# Generate corresponding network topology
cd ./src/ndnSIM/experiments
$PYTHON dcGenerator.py
cd ../../../

# Start simulation
NS_LOG=ndn.Consumer:ndn.ConsumerINA:ndn.Aggregator ./waf --run agg-aimd-test

# Generate simulation result
cd ./src/ndnSIM/experiments
$PYTHON ResultMeasurement.py