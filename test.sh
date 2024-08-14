#!/bin/bash

# Define the directories
TEXT_DIR="./src/ndnSIM/results/logs"
IMAGE_DIR="./src/ndnSIM/results/graphs"

# Function to remove files with a specific extension in a given directory
remove_files () {
    local directory=$1
    local extension=$2

    # Check if the directory exists
    if [ -d "$directory" ]; then
        # Remove all files with the specified extension in the directory
        echo "Removing all *.$extension files in $directory"
        rm -f $directory/*.$extension

        # Check if the removal was successful
        if [ $? -eq 0 ]; then
            echo "All *.$extension files have been removed successfully from $directory."
        else
            echo "Failed to remove *.$extension files from $directory."
        fi
    else
        echo "Directory does not exist: $directory"
    fi
}

# Remove .txt files from TEXT_DIR
remove_files "$TEXT_DIR" "txt"

# Remove .png files from IMAGE_DIR
remove_files "$IMAGE_DIR" "png"