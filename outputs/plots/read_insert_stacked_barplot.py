# Author: Amee Trivedi
# Objective: Stacked Plot bar graph of node and edge reads and insert. Also, plot of varying dataset size based on
#            a combination of type of data structure and Read-optimized option.

#imports
import pandas as pd
from matplotlib import pyplot as plt
import seaborn as sns


def main():
    df1 = pd.read_csv("/Users/amee/Desktop/Research/Puneet_Graphs/margraphita/outputs/merged_kron_insert.csv", index_col=False)
    print(df1.head(5))
    """
    sns.factorplot(x=' sum_out_deg', y=' time_taken_usecs', data=df2)
    plt.xticks(rotation=45)
    plt.ylim(-600,6000)
    plt.xlabel("Cummulative Outdegree of Visited Nodes", fontsize=15)
    plt.ylabel("Time(milliseconds)", fontsize=15)
    plt.tight_layout()
    plt.savefig("./adj_bfs_sum_outdegree_time.pdf")
    plt.show()
    """

if __name__ == "__main__":
    main()