#include "../include/BalancedKMeans.hpp"
#include "../include/Hungarian.hpp"
#include "../utility/utility.hpp"


#include <iostream>
#include <vector>
#include <cmath> // For ceil
#include <numeric> // For std::accumulate
#include <cassert> // For assert
#include <fstream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <limits>
#include <unordered_map>
#include <climits>
#include <set>

// Compare two clusters for balanced k-means algorithm
bool BalancedKMeans::compareClusters(const std::vector<std::vector<std::string>>& cluster1, const std::vector<std::vector<std::string>>& cluster2) {
    if (cluster1.size() != cluster2.size()) {
        return false; // Different number of clusters
    }

    // Iterate through each cluster
    for (size_t i = 0; i < cluster1.size(); ++i) {
        // Create copies of the inner vectors
        std::vector<std::string> cluster1_inner = cluster1[i];
        std::vector<std::string> cluster2_inner = cluster2[i];

        // Sort both inner vectors
        std::sort(cluster1_inner.begin(), cluster1_inner.end());
        std::sort(cluster2_inner.begin(), cluster2_inner.end());

        // Compare sorted vectors
        if (cluster1_inner != cluster2_inner) {
            return false; // Different elements in at least one corresponding pair of clusters
        }
    }
    return true; // All corresponding pairs of clusters contain the same elements
}


std::vector<std::vector<std::string>> BalancedKMeans::balancedKMeans(int N, int C, int numClusters, std::vector<int> clusterAssignment,
                                                                      std::vector<std::string> dataPointNames, std::vector<std::vector<std::string>> clusters, std::map<std::string, std::map<std::string, int>> linkCostMatrix) {
    // Vector to hold the output matrix in 1D format
    std::vector<int> output(N * N);

    // Calculate the matrix
    for (int i = 0; i < N; ++i) {
        int dataPoint = i;
        int clusterOfDataPoint = clusterAssignment[dataPoint];

        // "clusterNodes" stores the data points this cluster has
        const auto& clusterNodes = clusters[clusterOfDataPoint];

        for (int j = 0; j < N; ++j) {
            std::string otherDataPoint = dataPointNames[j];
            long long totalCost = 0;

            // Compute total cost between current data point and all nodes in this cluster
            for (const auto& node : clusterNodes) {
                int cost = linkCostMatrix[otherDataPoint][node];
                if (cost != -1)
                    totalCost += cost;
                else
                    throw std::runtime_error("Returned Link cost is -1, error!!!!!!!");
            }

            // Compute average cost and store it into output matrix
            int averageCost = static_cast<int>(totalCost / clusterNodes.size());
            output[i * N + j] = averageCost;
        }
    }

    // Output the matrix in 1D vector format
    std::cout << "\nCost Matrix:" << std::endl;
    for (int i = 0; i < N * N; ++i) {
        if (i % N == 0) std::cout << std::endl;  // New line for each row in visualization
        std::cout << output[i] << " ";
    }
    std::cout << std::endl;

    // Start of Hungarian algorithm
    // Cast data type into corresponding format in Hungarian
    const size_t MAX_SIZE = output.size();
    int Hungarian_input[MAX_SIZE];
    std::copy(output.begin(), output.end(), Hungarian_input);

    // Use Hungarian algorithm
    Hungarian hungarian;
    std::vector<int> allocation = hungarian.assignmentProblem(Hungarian_input, N);
    std::cout << "\nAllocation Matrix:" << std::endl;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            std::cout << allocation[i * N + j] << " ";
        }
        std::cout << std::endl;
    }

    // Update the cluster for next iteration
    std::vector<std::vector<std::string>> newCluster(numClusters);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++){
            if (allocation[i * N + j] == 1){
                newCluster[clusterAssignment[i]].push_back(dataPointNames[j]);
            }
        }
    }

    if (compareClusters(clusters, newCluster)){
        std::cout << "Hungarian algorithm converges." << std::endl;
        return newCluster;
    } else {

        // testing old cluster
        int i = 0;
        std::cout << "\nOld cluster" << std::endl;
        for (const auto& oldItemVector : clusters) {
            std::cout << "Cluster " << i << " " <<std::endl;
            for (const auto& oldStr : oldItemVector) {
                std::cout << oldStr << " ";
            }
            std::cout << std::endl;
            ++i;
        }

        // testing new cluster
        int j = 0;
        std::cout << "\nNew cluster" << std::endl;
        for (const auto& oldItemVector : newCluster) {
            std::cout << "Cluster " << j << " " <<std::endl;
            for (const auto& oldStr : oldItemVector) {
                std::cout << oldStr << " ";
            }
            std::cout << std::endl;
            ++j;
        }

    }
    balancedKMeans(N, C, numClusters, clusterAssignment, dataPointNames, clusters, linkCostMatrix);
}