#!/bin/bash

# Generate corresponding network topology
 python3 src/ndnSIM/experiments/dcGenerator.py

# Start simulation
NS_LOG=ndn.Consumer:ndn.ConsumerINA:ndn.Aggregator ./waf --run agg-aimd-test

# Generate simulation result
 python3 src/ndnSIM/experiments/ResultMeasurement.py
