#!/bin/bash

# Check if a Python path argument was provided
if [ -z "$1" ]; then
  echo "Usage: $0 <path_to_python_executable>"
  exit 1
fi

# Use the provided argument for the Python path
PYTHON="$1"

# Generate corresponding network topology
cd ./src/ndnSIM/experiments
$PYTHON dcGenerator.py
cd ../../../

# Start simulation
NS_LOG=ndn.Consumer:ndn.ConsumerINA:ndn.Aggregator ./waf --run agg-aimd-test

# Generate simulation result
cd ./src/ndnSIM/experiments
$PYTHON ResultMeasurement.py
