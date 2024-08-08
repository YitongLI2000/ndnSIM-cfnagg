

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
#include <algorithm>

class BalancedKMeans {
public:

    bool compareClusters(const std::vector<std::vector<std::string>>& cluster1, const std::vector<std::vector<std::string>>& cluster2);

    std::vector<std::vector<std::string>> balancedKMeans(int N, int C, int numClusters, std::vector<int> clusterAssignment,
                                                         std::vector<std::string> dataPointNames, std::vector<std::vector<std::string>> clusters, std::map<std::string, std::map<std::string, int>> linkCostMatrix);
};