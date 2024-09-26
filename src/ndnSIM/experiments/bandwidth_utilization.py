import configparser
import re
import sys


def get_bitrate():
    config = configparser.ConfigParser()
    config.read('config.ini')

    # Mapping to convert corresponding units into number
    unit_conversion = {'bps':1,
                       'Kbps':1000,
                       'Mbps':1000000,
                       'Gbps':1000000000}

    topology_type = config['General']['TopologyType']
    topology = str(topology_type + "Topology")
    pattern = re.compile(r"(^\d+)(bps|Kbps|Mbps|Gbps)$")
    result = pattern.match(config[topology]['Bitrate'])

    if result is not None:
        number = result.group(1)
        unit = result.group(2)
        bitrate = int(number) * unit_conversion[unit]
        return bitrate
    else:
        print(f"Input bitrate {config[topology]['Bitrate']} is not valid")
        return 0


def compute_throughput(filename):
    with open(filename, 'r') as file:
        lines = file.readlines()

    total_throughput = 0
    total_links = 0

    # Read each row and calculate throughput
    for line_index, line in enumerate(lines, 1):
        if line.strip():  # check if line is not empty
            interest_packet_size, data_packet_size, number_of_links, start_time, stop_time = map(int, line.split())
            total_time = stop_time - start_time  # in milliseconds
            #total_bits = (interest_packet_size + data_packet_size) * 8  # total bits transmitted, including interest and data
            total_bits = data_packet_size * 8 # only count data size, ignore interest

            # Convert microseconds to seconds for throughput calculation in bits per second
            if total_time > 0:
                throughput = total_bits / (total_time / 1000000)
                if throughput == 0:
                    raise ValueError(f"Error: Throughput for node on line {line_index} is computed as 0 bps. Please check input data.")
            else:
                raise ValueError(f"Error: Invalid total time (stop time - start time must be positive) for node on line {line_index}.")

            total_throughput += throughput
            total_links += number_of_links

    # Compute theoretical max throughput
    # Bitrate per link is 100 Mbps, converting Mbps to bps by multiplying by 10^6
    bitrate_per_link = get_bitrate()
    if bitrate_per_link == 0:
        raise ValueError("Error: Bitrate can't be parsed correctly.")

    theoretical_max_throughput = total_links * bitrate_per_link

    # Compute total bandwidth utilization
    if theoretical_max_throughput > 0:
        total_bandwidth_utilization = total_throughput / theoretical_max_throughput
    else:
        raise ValueError("Error: Theoretical maximum throughput is 0, indicating no links are available.")

    return total_throughput, theoretical_max_throughput, total_bandwidth_utilization


if __name__ == "__main__":
    try:
        total_throughput, theoretical_max_throughput, total_bandwidth_utilization = compute_throughput("../results/logs/throughput.txt")
        print("Total Throughput (bps):", total_throughput)
        print("Theoretical Max Throughput (bps):", theoretical_max_throughput)
        print("Total Bandwidth Utilization (%):", total_bandwidth_utilization * 100)
    except ValueError as e:
        print(e)
        sys.exit(1)