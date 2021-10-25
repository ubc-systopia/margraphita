# Author: Amee Trivedi
# Objective: Plot std_rd_cit-Patents_bfs run time for various nodes with varying outdegree

#imports
import pandas as pd
from matplotlib import pyplot as plt
import seaborn as sns


def main():
    df1 = pd.read_csv("/Users/amee/Desktop/Research/Puneet_Graphs/margraphita/outputs/std_rd_cit-Patents_bfs.csv", index_col=False)
    df1["Type"] = "Std"
    df1["Time"] = df1[" time_taken_usecs"]/1000
    df2 = pd.read_csv("/Users/amee/Desktop/Research/Puneet_Graphs/margraphita/outputs/adj_rd_cit-Patents_bfs.csv", index_col=False)
    df2["Type"] = "Adj"
    df2["Time"] = df2[" time_taken_usecs"]/1000
    frames = [df1, df2]
    df = pd.concat(frames)
    print(df.head(5))
    print(list(df))
    sns.factorplot(x=' num_visited', y=' time_taken_usecs', data=df2)
    plt.xticks(rotation=45)
    plt.ylim(-600,6000)
    plt.xlabel("Count(Nodes Visited)", fontsize=15)
    plt.ylabel("Time ("r"$\mu$" "sec)", fontsize=15)
    plt.tight_layout()
    plt.savefig("./adj_bfs_time.pdf")
    plt.show()

    sns.factorplot(x=' sum_out_deg', y=' time_taken_usecs', data=df2)
    plt.xticks(rotation=45)
    plt.ylim(-600,6000)
    plt.xlabel("Cummulative Outdegree of Visited Nodes", fontsize=15)
    plt.ylabel("Time(milliseconds)", fontsize=15)
    plt.tight_layout()
    plt.savefig("./adj_bfs_sum_outdegree_time.pdf")
    plt.show()

if __name__ == "__main__":
    main()