# Author: Amee Trivedi
# Objective: To plot formatting and insert times for various graph sizes as determined by the value of s

import numpy as np
import pandas as pd
import seaborn as sns
from matplotlib import pyplot as plt


def compute_speed(row):
    return(row["Disk_Space"]*1.0/row["Real_Time"])

def main():
    df = pd.read_csv("/Users/amee/Desktop/Research/Puneet_Graphs/formatting_insert_time.csv")
    print(list(df))
    print(df)
    df["Disk_Space"] = 50000
    print(df)

    # Compute the speed using Disk_Space and Real_Time
    df["Speed"] = df.apply(compute_speed, axis=1)
    print(df)

    # Average speed of the ssd
    ssd_speed = [360,359,363,359]
    avg_speed = sum(ssd_speed)/len(ssd_speed)
    print(avg_speed)

    # Plot
    sns.set_style("white")
    sns.lineplot(data=df, x="S", y="Speed", hue="Type", style="Type")

    # Dotted reference line on the plot
    x = np.arange(int(df["S"].min()), int(df["S"].max()+1))
    print(x)
    y = [avg_speed]*len(x)
    print(y)
    plt.plot(x,y, linestyle="-.", label="SSD Baseline", color="green")

    plt.legend()
    plt.xlabel("S Value", fontsize=18)
    plt.ylabel("Speed (MB/s)", fontsize=18)
    plt.tight_layout()
    plt.savefig("./format_insert.pdf")
    plt.show()

    return(0)

if __name__ == "__main__":
    main()
