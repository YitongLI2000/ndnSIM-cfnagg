#!/bin/bash

GRAPH_DIR="./src/ndnSIM/results/graphs"
LOGS_DIR="./src/ndnSIM/results/logs"

# Define function to clear all contents from specific path
clear_directory() {
    local directory=$1

    # Check if directory exists
    if [ -d "$directory" ]; then
        # Remove all contents within the directory
        echo "Removing all contents in $directory"
        rm -rf $directory/*

        # Check if the removal was successful
        if [ $? -eq 0 ]; then
            echo "All contents have been removed successfully from $directory."
        else
            echo "Failed to remove contents from $directory."
        fi
    else
        echo "Directory does not exist: $directory, no need for further operation."
    fi
}

# Remove everything from specific folders
clear_directory "$GRAPH_DIR"
clear_directory "$LOGS_DIR"

# Specify python version
PYTHON_VERSION="python3.10"

# Use the provided argument to find the Python executable path
PYTHON=$(which "$PYTHON_VERSION")

# Check if the Python executable was found
if [ -z "$PYTHON" ]; then
  echo "Error: Python executable not found for version $PYTHON_VERSION"
  exit 1
fi

# Generate corresponding network topology, DCN
cd ./src/ndnSIM/experiments
CONFIG_FILE="config.ini"
TOPOLOGY_TYPE=$(grep "TopologyType" $CONFIG_FILE | awk -F "= " '{print $2}')

if [ "$TOPOLOGY_TYPE" == "DCN" ]; then
    echo "TopologyType is DCN. Generating DC network..."
    $PYTHON dcGenerator.py
else
    echo "TopologyType is not DCN." # Replace it with another ISP network later
    exit 1
fi

# Check whether topology is generated successfully
if [ $? -ne 0 ]; then
    echo "Fail to generate topology, please check all input parameters!"
    exit 1
fi

cd ../../../

# Start simulation
NS_LOG=ndn.Consumer:ndn.ConsumerINA:ndn.Aggregator ./waf --run agg-aimd-test
#NS_LOG=ndn.Consumer:ndn.ConsumerINA ./waf --run agg-aimd-test


# Generate simulation result
cd ./src/ndnSIM/experiments
$PYTHON bandwidth_utilization.py

# Check whether bandwidth utilization is computed successfully
if [ $? -ne 0 ]; then
    echo "Fail to compute bandwidth utilization, please check again!"
    exit 1
fi

#$PYTHON throughput_measurement.py
#$PYTHON consumer_result_generator.py
#$PYTHON agg_result_generator.py
