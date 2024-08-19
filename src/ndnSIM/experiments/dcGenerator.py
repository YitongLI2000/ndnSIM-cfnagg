import os
import sys
import re
import configparser

num_core_forwarders = 3




def is_valid_bitrate(bitrate):
    pattern = re.compile(r"^\d+(Kbps|Mbps|Gbps|bps)$")
    return pattern.match(bitrate) is not None

def generate_topology(num_producers, num_aggregators, num_producers_per_edge, bit_rate):
    num_edge_forwarders = (num_producers // num_producers_per_edge) + 1

    # open the topology txt file
    # with open("/home/dd/agg-ndnSIM/ns-3/src/ndnSIM/examples/topologies/DataCenterTopology.txt", "w") as output_file:
    with open("../examples/topologies/DataCenterTopology.txt", "w") as output_file:
        # write "router" section
        output_file.write("router\n\n")
        output_file.write("con0\n")
        for i in range(num_producers):
            output_file.write(f"pro{i}\n")

        for i in range(num_edge_forwarders):
            output_file.write(f"forwarder{i}\n")

        for i in range(num_aggregators):
            output_file.write(f"agg{i}\n")

        for i in range(num_core_forwarders):
            output_file.write(f"forwarder{num_edge_forwarders + i}\n")

        # write "link" section
        output_file.write("\nlink\n\n")

        for i in range(num_producers):
            output_file.write(f"pro{i}       forwarder{i // num_producers_per_edge}       {bit_rate}       1       2ms       50\n")

        output_file.write(f"con0       forwarder0       {bit_rate}       1       2ms       50\n")

        for i in range(num_edge_forwarders):
            for j in range(num_aggregators):
                output_file.write(f"forwarder{i}       agg{j}       {bit_rate}       1       2ms       50\n")

        for i in range(num_core_forwarders):
            for j in range(num_aggregators):
                output_file.write(f"forwarder{num_edge_forwarders + i}       agg{j}       {bit_rate}       1       2ms       50\n")


def main():
    # Read information from config.ini
    config = configparser.ConfigParser()
    config.read('config.ini')
    num_producers = int(config['DCNTopology']['numProducer'])
    num_aggregators = int(config['DCNTopology']['numAggregator'])
    number_producer_per_edge = int(config['DCNTopology']['numEdgeForwarder'])
    bit_rate = config['DCNTopology']['Bitrate']

    print("Current Working Directory:", os.getcwd())

    if not is_valid_bitrate(bit_rate):
        print(f"Error! Bitrate '{bit_rate}' is not in the correct format. Only the following format is allowed:\n1bps, 1Kbps, 1Mbps, 1Gbps")
        sys.exit(1)

    generate_topology(num_producers, num_aggregators, number_producer_per_edge, bit_rate)

    print("Topology generated successfully!")

if __name__ == "__main__":
    main()
