import os

# File format: "interestThroughput", "dataThroughput", "Simulation stop time"
# The third column "Simulation stop time", only consumer will record this, aggregator will record 0 for this column


def compute_throughput():
    # Define the path to your file
    file_path = '../results/logs/throughput.txt'

    # Check whether the file exists
    if not os.path.exists(file_path):
        print(f"Error! The {file_path} doesn't exist!")
        return

    # A list to store the sum of each column. Initialize with `None` to handle the first line dynamically
    column_sums = None

    # Open the file and read the contents
    with open(file_path, 'r') as file:
        for line in file:
            # Split each line into parts using space as the delimiter
            columns = line.strip().split()

            # Convert each column value to a float
            columns = [float(column) for column in columns]

            # Initialize the column_sums list with zeros if it is the first line
            if column_sums is None:
                column_sums = [0] * len(columns)

            # Add each column's value to its corresponding sum
            for i, value in enumerate(columns):
                column_sums[i] += value

    # Compute throughput
    interest = column_sums[0] * 8
    data = column_sums[1] * 8
    time = column_sums[2]
    throughput = int((interest + data) / time)

    print("Total interest packet size: ", "{:,}".format(int(interest)), " bits.")
    print("Total data packet size: ", "{:,}".format(int(data)), " bits.")
    print("Total throughput: ", "{:,}".format(throughput), " bits/s")


if __name__ == "__main__":
    compute_throughput()
