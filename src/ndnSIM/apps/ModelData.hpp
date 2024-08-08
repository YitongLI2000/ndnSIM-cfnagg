#pragma once

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/callback.h"
#include <vector>
#include <cstdint>

struct ModelData {
    std::vector<float> parameters;
    std::vector<std::string> congestedNodes;

    ModelData();
};

void serializeModelData(const ModelData& modelData, std::vector<uint8_t>& buffer);
bool deserializeModelData(const std::vector<uint8_t>& buffer, ModelData& modelData);
