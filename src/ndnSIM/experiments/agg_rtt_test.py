import pandas as pd
import matplotlib.pyplot as plt
import os
import glob

def draw_rtt_graphs(file):
    """
    Draw a graph for RTT data from a given log file.

    :param file: Path to the log file.
    :return: Status message.
    """
    try:
        print("Loading data from:", file)
        data = pd.read_csv(file, sep="\s+", header=None)
        data.columns = ['Time', 'RTT']

        fig, ax = plt.subplots()
        ax.plot(data['Time'], data['RTT'], color='tab:red')
        ax.set_xlabel('Time')
        ax.set_ylabel('RTT')
        plt.title(f"RTT for {os.path.basename(file)}")
        plt.grid(True)

        # Save the plot with a dynamic name based on the file
        output_path = os.path.join(os.path.dirname(file), f"./result/{os.path.basename(file).split('.')[0]}.png")
        plt.savefig(output_path)
        plt.close()

        return f"Plot created and saved successfully at {output_path}."
    except Exception as e:
        return f"An error occurred: {str(e)}"

def main():
    log_folder = "../examples/log/"
    aggregator_files = glob.glob(os.path.join(log_folder, "agg*_RTT.txt"))

    # print("Found aggregator files:", aggregator_files)

    results = []
    for file in aggregator_files:
        result = draw_rtt_graphs(file)
        results.append(result)
        print(result)

if __name__ == "__main__":
    main()
