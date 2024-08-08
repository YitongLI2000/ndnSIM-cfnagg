#include <iostream>
#include <fstream>
#include <vector>
#include <string>

const int NUM_CORE_FORWARDERS = 3;
//const int NUM_EDGE_FORWARDERS_PER_PRODUCER = 5; // One forwarder connects to 5 producers
//const int NUM_AGG_CONNECTIONS_PER_EDGE = 10; // You can adjust this value

void generateTopology(int numProducers, int numAggregators, int numProducerPerEdge) {
    int numEdgeForwarders = (numProducers / numProducerPerEdge) + 1;

    // Open the output file
    std::ofstream outFile("/home/dd/agg-ndnSIM/ns-3/src/ndnSIM/examples/topologies/DataCenterTopology.txt");

    // Write the routers section
    outFile << "router\n\n";

    // Consumer
    outFile << "con0\n";

    // Producers
    for (int i = 0; i < numProducers; ++i) {
        outFile << "pro" << i << "\n";
    }

    // Forwarders at edge
    for (int i = 0; i < numEdgeForwarders; ++i) {
        outFile << "forwarder" << i << "\n";
    }

    // Aggregators
    for (int i = 0; i < numAggregators; ++i) {
        outFile << "agg" << i << "\n";
    }

    // Core forwarders
    for (int i = 0; i < NUM_CORE_FORWARDERS; ++i) {
        outFile << "forwarder" << numEdgeForwarders + i << "\n";
    }

    outFile << "\nlink\n\n";

    // Links from producers and consumer to edge forwarders
    for (int i = 0; i < numProducers; ++i) {
        outFile << "pro" << i << "       forwarder" << (i / numProducerPerEdge) << "       1Mbps       1       2ms       100\n";
    }
    // Consumer to a dedicated edge forwarder
    outFile << "con0       forwarder0       1Mbps       1       2ms       100\n";

    // Links from edge forwarders to aggregation forwarders
    for (int i = 0; i < numEdgeForwarders; ++i) {
        for (int j = 0; j < numAggregators; ++j) {
            outFile << "forwarder" << i << "       agg" << j << "       1Mbps       1       2ms       100\n";
        }
    }

    // Links from core forwarders to aggregators
    for (int i = 0; i < NUM_CORE_FORWARDERS; ++i) {
        for (int j = 0; j < numAggregators; ++j) {
            outFile << "forwarder" << numEdgeForwarders + i << "       agg" << j << "       3Mbps       1       2ms       100\n";
        }
    }

    // Close the file
    outFile.close();
}

int main() {
    int numProducers, numAggregators, numProducerPerEdge;
    std::cout << "Enter the number of producers: ";
    std::cin >> numProducers;
    std::cout << "Enter the number of aggregators: ";
    std::cin >> numAggregators;
    std::cout << "Enter the number of producers connected with one edge forwarder: ";
    std::cin >> numProducerPerEdge;

    generateTopology(numProducers, numAggregators, numProducerPerEdge);

    std::cout << "Topology file has been generated." << std::endl;

    return 0;
}
