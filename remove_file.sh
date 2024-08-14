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