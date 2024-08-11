#!/bin/bash

# Generate corresponding network topology
cd ./src/ndnSIM/experiments
python3 dcGenerator.py
cd ../../../

# Start simulation
NS_LOG=ndn.Consumer:ndn.ConsumerINA:ndn.Aggregator ./waf --run agg-aimd-test

# Generate simulation result
cd ./src/ndnSIM/experiments
python3 ResultMeasurement.py
