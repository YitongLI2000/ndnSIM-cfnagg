import pandas as pd
import matplotlib.pyplot as plt
import os
import glob


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


def aggregator_rto_aggregationtime(file1, file2):
    """
    Draw graph for aggregator, x: Time y: RTO/Aggregation time
    Two graphs are drown, two individual graphs for RTO/Aggregation time

    :param file1: file contains Time, RTO
    :param file2: File contains Time, Aggregation time
    :return: Status message.
    """
    try:
        # Load data from txt file
        data1 = pd.read_csv(file1, sep="\s+", header=None)
        data1.columns = ['Time', 'RTO']

        data2 = pd.read_csv(file2, sep="\s+", header=None)
        data2.columns = ['Time', 'Aggregation time']

        # Extract aggregator's name
        filename = os.path.basename(file1) # Extract "agg*_RTO.txt"
        aggname = filename.split('_RTO')[0] # Extract "agg*"

        # Create individual graph for rto and aggregation time
        plt.figure(figsize=(10, 6))
        plt.plot(data1['Time'], data1['RTO'], 'tab:red', alpha=0.8, linewidth=1, label='RTO')
        plt.xlabel('Time')
        plt.ylabel('RTO')
        plt.title(f"{aggname}: RTO")
        plt.grid(True)
        plt.legend()
        window_path = os.path.join(os.path.dirname(file1), f"../graphs/agg/{aggname}_rto.png")
        plt.savefig(window_path, dpi=300)
        plt.close()

        plt.figure(figsize=(10, 6))
        plt.plot(data2['Time'], data2['Aggregation time'], 'tab:blue', alpha=0.8, linewidth=1, label='Aggregation time')
        plt.xlabel('Time')
        plt.ylabel('Aggregation time')
        plt.title(f"{aggname}: Aggregation time")
        plt.grid(True)
        plt.legend()
        rtt_path = os.path.join(os.path.dirname(file1), f"../graphs/agg/{aggname}_aggregationTime.png")
        plt.savefig(rtt_path, dpi=300)
        plt.close()

        return f"Individual rto and aggregation time graph for {aggname} are created and saved successfully."
    except Exception as e:
        return f"An error occurred: {str(e)}"




def aggregator_window_rtt(file1, file2):
    """
    Draw graph for aggregator, x: Time y: Window vs. RTT
    Three graphs are drown, combined graph for Window vs. RTT; two individual graphs for Window/RTT

    :param file1: file contains Time, Window
    :param file2: File contains Time, RTT
    :return: Status message.
    """
    try:
        # Load data from txt file
        data1 = pd.read_csv(file1, sep="\s+", header=None)
        data1.columns = ['Time', 'Window']

        data2 = pd.read_csv(file2, sep="\s+", header=None)
        data2.columns = ['Time', 'RTT']

        fig, ax1 = plt.subplots(figsize=(10, 6))

        # Create a combined graph for window/rtt
        # First column for Window
        ax1.plot(data1['Time'], data1['Window'], color='tab:red', alpha=0.7, linewidth=1)
        ax1.set_xlabel('Time')
        ax1.set_ylabel('Window')
        ax1.tick_params(axis='y', labelcolor='tab:red')

        # Second column for RTT
        ax2 = ax1.twinx()
        ax2.set_ylabel("RTT", color="tab:blue")
        ax2.plot(data2['Time'], data2['RTT'], color="tab:blue", alpha=0.7, linewidth=1, linestyle='--')
        ax2.tick_params(axis='y', labelcolor='tab:blue')

        # Extract aggregator's name
        filename = os.path.basename(file1) # Extract "agg*_window.txt"
        aggname = filename.split('_window')[0] # Extract "agg*"

        plt.title(f"{aggname}: Window vs. RTT")
        plt.grid(True)

        # Save the plot with a dynamic name based on the file
        output_path = os.path.join(os.path.dirname(file1), f"../graphs/agg/{aggname}_window_rtt.png")
        plt.savefig(output_path, dpi=300)
        plt.close()

        # Create individual graph for window and rtt
        plt.figure(figsize=(10, 6))
        plt.plot(data1['Time'], data1['Window'], 'tab:red', alpha=0.8, linewidth=1, label='Window')
        plt.xlabel('Time')
        plt.ylabel('Window')
        plt.title(f"{aggname}: Window")
        plt.grid(True)
        plt.legend()
        window_path = os.path.join(os.path.dirname(file1), f"../graphs/agg/{aggname}_window.png")
        plt.savefig(window_path, dpi=300)
        plt.close()

        plt.figure(figsize=(10, 6))
        plt.plot(data2['Time'], data2['RTT'], 'tab:blue', alpha=0.8, linewidth=1, label='RTT')
        plt.xlabel('Time')
        plt.ylabel('RTT')
        plt.title(f"{aggname}: RTT")
        plt.grid(True)
        plt.legend()
        rtt_path = os.path.join(os.path.dirname(file1), f"../graphs/agg/{aggname}_rtt.png")
        plt.savefig(rtt_path, dpi=300)
        plt.close()

        return f"Combined/individual window and rtt graph for {aggname} are created and saved successfully."
    except Exception as e:
        return f"An error occurred: {str(e)}"

def main():
    # Check whether output path exists
    output_path = "../results/graphs/agg"
    check_and_create_dir(output_path)

    # Collate log files
    log_path = "../results/logs"
    files_rtt = glob.glob(os.path.join(log_path, "agg*_RTT.txt"))
    files_window = glob.glob(os.path.join(log_path, "agg*_window.txt"))
    files_rto = glob.glob(os.path.join(log_path, "agg*_RTO.txt"))
    files_aggregationtime = glob.glob(os.path.join(log_path, "agg*_aggregationTime.txt"))

    if not files_rtt:
        print("No RTT log files found.")
        return
    if not files_window:
        print("No window log files found.")
        return
    if not files_rto:
        print("No RTO log files found.")
        return
    if not files_aggregationtime:
        print("No aggregation time log files found.")
        return

    dict_rtt = {os.path.basename(f).split('_')[0]: f for f in files_rtt} # {"agg*", rtt_file_path}
    dict_window = {os.path.basename(f).split('_')[0]: f for f in files_window} # {"agg*", window_file_path}
    dict_rto = {os.path.basename(f).split('_')[0]: f for f in files_rto} # {"agg*", rto_file_path}
    dict_aggregationtime = {os.path.basename(f).split('_')[0]: f for f in files_aggregationtime} # {"agg*", aggregationtime_file_path}

    for agg_id, rtt_file in dict_rtt.items():
        window_file = dict_window.get(agg_id)
        rto_file = dict_rto.get(agg_id)
        aggregationtime_file = dict_aggregationtime.get(agg_id)
        if window_file:
            result = aggregator_window_rtt(window_file, rtt_file)
            print(result)
        else:
            print(f"No matching window file found for {window_file}/{rtt_file}")

        if rto_file and aggregationtime_file:
            result = aggregator_rto_aggregationtime(rto_file, aggregationtime_file)
            print(result)
        else:
            print(f"No matching window file found for {rto_file}/{aggregationtime_file}")

if __name__ == "__main__":
    main()
