import pandas as pd
import matplotlib.pyplot as plt
import os



def check_and_create_dir(path):
    """
    Check if the specified directory exists, and create it if it does not.

    :param path: Path to the directory to check and possibly create.
    :return: None
    """
    if not os.path.exists(path):
        os.makedirs(path)
        print(f"Directory created at: {path}")
    else:
        print(f"Directory already exists at: {path}")


def Consumer_rtt_aggregationTime(file1, file2):
    """
    Draw a graph for consumer, x: Time y: RTT vs. aggregation time
    Data is captured when data packet returned
    Aggregation time is recorded when each iteration finished

    :param file1: Path to the file containing Time, seq, and RTT data.
    :param file2: Path to the file containing Time and Aggregation time data.
    :return: Status message indicating success or describing an error.
    """
    try:
        # Load data from txt file
        # print("Loading data from: ", file1)
        data1 = pd.read_csv(file1, sep="\s+", header=None, usecols=[0, 4])
        data1.columns = ['Time', 'RTT']

        data2 = pd.read_csv(file2, sep="\s+", header=None)
        data2.columns = ['Time', 'Aggregation time']

        # Create a figure with a single axis
        fig, ax1 = plt.subplots(figsize=(10, 6))

        # First y-axis for RTT
        color = 'tab:red'
        ax1.set_xlabel('Time')
        ax1.set_ylabel('RTT', color=color)
        ax1.plot(data1['Time'], data1['RTT'], color=color, alpha=0.8, linewidth=1)
        ax1.tick_params(axis='y', labelcolor=color)

        # Second y-axis for aggregation time
        ax2 = ax1.twinx()
        color = 'tab:blue'
        ax2.set_ylabel("Aggregation time", color=color)
        ax2.plot(data2['Time'], data2['Aggregation time'], color=color, alpha=0.8, linewidth=1, linestyle='--')
        ax2.tick_params(axis='y', labelcolor=color)

        plt.title("Consumer: RTT vs. Aggregation time")
        plt.grid(True)
        plt.savefig("../results/graphs/con/consumer_rtt_aggregationTime.png", dpi=300)
        plt.close()

        return "Plot 'Consumer: RTT vs. Aggregation time' created and saved successfully."
    except FileNotFoundError as e:
        return f"File not found: {e}"
    except pd.errors.EmptyDataError as e:
        return f"One of the files is empty: {e}"
    except Exception as e:
        return f"An error occurred: {str(e)}"



def Consumer_window_rtt(file1, file2):
    """
    Draw a graph for consumer, x: Time y: window vs. RTT

    :param file1: Path to the file containing Time, window.
    :param file2: Path to the file containing Time, RTT.
    :return: Status message indicating success or describing an error.
    """
    try:
        # Load data from txt file
        data1 = pd.read_csv(file1, sep="\s+", header=None, usecols=[0, 1])
        data1.columns = ['Time', 'Window']

        data2 = pd.read_csv(file2, sep="\s+", header=None, usecols=[0, 4])
        data2.columns = ['Time', 'RTT']

        # Create a figure with a single axis
        fig, ax1 = plt.subplots(figsize=(10, 6))

        # First y-axis for Window
        color = 'tab:red'
        ax1.set_xlabel('Time')
        ax1.set_ylabel('Window', color=color)
        ax1.plot(data1['Time'], data1['Window'], color=color, alpha=0.8, linewidth=1)
        ax1.tick_params(axis='y', labelcolor=color)

        # Second y-axis for RTT
        ax2 = ax1.twinx()
        color = 'tab:blue'
        ax2.set_ylabel("RTT", color=color)
        ax2.plot(data2['Time'], data2['RTT'], color=color, alpha=0.8, linewidth=1, linestyle='--')
        ax2.tick_params(axis='y', labelcolor=color)

        plt.title("Consumer: Window vs. RTT")
        plt.grid(True)
        plt.savefig("../results/graphs/con/consumer_window_rtt.png", dpi=300)
        plt.close()

        return "Plot 'Consumer: Window vs. RTT' created and saved successfully."
    except FileNotFoundError as e:
        return f"File not found: {e}"
    except pd.errors.EmptyDataError as e:
        return f"One of the files is empty: {e}"
    except Exception as e:
        return f"An error occurred: {str(e)}"




def Consumer_window_rto(file1, file2):
    """
    Draw a graph for consumer, x: Time y: Window vs. RTO
    Data is captured every 5 ms

    :param file1:
    :param file2:
    :return:
    """
    try:
        # Load data from txt files
        # print("Loading data from:", file1)
        data1 = pd.read_csv(file1, sep="\s+", header=None, usecols=[0, 1])
        data1.columns = ['Time', 'Window']

        # print("Loading data from:", file2)
        data2 = pd.read_csv(file2, sep="\s+", header=None)
        data2.columns = ['Time', "RTO"]

        # Create a new figure
        fig, ax1 = plt.subplots(figsize=(10, 6))

        # First y-axis for RTO
        color = 'tab:red'
        ax1.set_xlabel('Time')
        ax1.set_ylabel('RTO', color=color)
        ax1.plot(data2['Time'], data2['RTO'], color=color, alpha=0.8, linewidth=1)
        ax1.tick_params(axis='y', labelcolor=color)

        # Second y-axis for Window
        ax2 = ax1.twinx()
        color = 'tab:blue'
        ax2.set_ylabel("Window", color=color)
        ax2.plot(data1['Time'], data1['Window'], color=color, alpha=0.8, linewidth=1, linestyle='--')
        ax2.tick_params(axis='y', labelcolor=color)

        plt.title("Consumer: Window vs. RTO")
        plt.grid(True)
        plt.savefig("../results/graphs/con/consumer_window_rto.png", dpi=300)
        plt.close()

        return "Plot 'Consumer: Window vs. RTO' created and saved successfully."
    except FileNotFoundError as e:
        return f"File not found: {e}"
    except pd.errors.EmptyDataError as e:
        return f"One of the files is empty: {e}"
    except Exception as e:
        return f"An error occurred: {str(e)}"



def main():
    # Print the current working directory
    print("Current Working Directory:", os.getcwd())

    file1 = "../results/logs/consumer_window.txt"
    file2 = "../results/logs/consumer_RTO.txt"
    file3 = "../results/logs/consumer_RTT.txt"
    file4 = "../results/logs/consumer_aggregationTime.txt"

    # Check if files exist
    if not os.path.exists(file1):
        print(f"Error, {file1} doesn't exist.")
        return
    if not os.path.exists(file2):
        print(f"Error, {file2} doesn't exist.")
        return
    if not os.path.exists(file3):
        print(f"Error, {file3} doesn't exist.")
        return
    if not os.path.exists(file4):
        print(f"Error, {file4} doesn't exist.")
        return

    # Check whether output path exists
    output_path = "../results/graphs/con"
    check_and_create_dir(output_path)

    # Return execution log
    result1 = Consumer_window_rto(file1, file2)
    print(result1)
    result2 = Consumer_rtt_aggregationTime(file3, file4)
    print(result2)
    result3 = Consumer_window_rtt(file1, file3)
    print(result3)


if __name__ == "__main__":
    main()
