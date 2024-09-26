#include "ModelData.hpp"
#include <cstring>
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/callback.h"

/**
 * Constructor
 */
ModelData::ModelData()
: parameters(3000, 0.0f)
{}



/**
 * Transfer the data's content (in the format of struct) to adapt ndn::Buffer object (this object's format is std::vector<uint_8>)
 * @param modelData Input struct
 * @param buffer ndn::Buffer format (can be considered as the output)
 */
void serializeModelData(const ModelData& modelData, std::vector<uint8_t>& buffer){
    // Clear the buffer first
    buffer.clear();

    // Transfer ModelData.parameters into bytes first
    size_t paramSize = modelData.parameters.size() * sizeof(float);
    buffer.resize(paramSize);
    std::memcpy(buffer.data(), modelData.parameters.data(), paramSize);


    // Transfer ModelData.congestedNodes into bytes
    for (const auto& str : modelData.congestedNodes) {
        uint32_t strLength = static_cast<uint32_t>(str.size());
        buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&strLength), reinterpret_cast<uint8_t*>(&strLength + 1)); // Insert the length of each string within the vector
        buffer.insert(buffer.end(), str.begin(), str.end()); // Insert the string
    }
}



/**
 * Transfer the ndn::Buffer object back into struct format, extract information
 * @param buffer Input ndn::Buffer object, this is input
 * @param modelData Original struct, this is output
 * @return If operation finishes successfully, return true
 */
bool deserializeModelData(const std::vector<uint8_t>& buffer, ModelData& modelData){

    // Transfer ModelData.parameters back first
    size_t paramSize = modelData.parameters.size() * sizeof(float);
    if (buffer.size() < paramSize){
        std::cout << "Buffer size is smaller than expected!" << std::endl;
        return false;
    }
    std::memcpy(modelData.parameters.data(), buffer.data(), paramSize);


    // Transfer ModelData.congestedNodes back
    size_t currentIndex = paramSize;
    while (currentIndex < buffer.size()) {
        if (currentIndex + sizeof(uint32_t) > buffer.size()) {
            std::cout << "Buffer size can't hold string length!" << std::endl;
            return false;
        }

        uint32_t strLength;
        std::memcpy(&strLength, buffer.data() + currentIndex, sizeof(uint32_t)); // Copy memory of the string's length
        currentIndex += sizeof(uint32_t);

        if (currentIndex + strLength > buffer.size()) {
            std::cout << "Buffer size can't hold string content!" << std::endl;
            return false;
        }
        std::string str(reinterpret_cast<const char*>(buffer.data() + currentIndex), strLength);
        modelData.congestedNodes.push_back(str);
        currentIndex += strLength;
    }

    return true;
}
