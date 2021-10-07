# Author: Amee Trivedi
# Objective: Stacked Plot bar graph of node and edge reads and insert. Also, plot of varying dataset size based on
#            a combination of type of data structure and Read-optimized option.

#imports
import pandas as pd
from matplotlib import pyplot as plt
import seaborn as sns
import matplotlib.cm as cm
import numpy as np
import matplotlib.pyplot as plt


def get_s_value(row):
    #s10_e8/adj_d_s10_e8
    items = row["name"].split("_")
    s_value = int(items[0].lstrip("s"))
    type_val = row["name"].split("/")[1].split("_s")[0]
    print(s_value, type_val)
    return pd.Series([s_value, type_val])


def main():
    df1 = pd.read_csv("/Users/amee/Desktop/Research/Puneet_Graphs/margraphita/outputs/merged_kron_insert.csv", index_col=False)
    print(df1.head(5))
    df1[["S", "Type"]] = df1.apply(get_s_value, axis=1)
    print(df1.head(5))

    df_wide = df1.pivot("S", "Type", "total_size")
    print(df_wide.head())
    sns.lineplot(data=df_wide)
    plt.xlabel("S Value", fontsize=18)
    plt.ylabel("Dataset Size(Bytes)", fontsize=18)
    plt.tight_layout()
    plt.savefig("./dataset_size_lineplot.pdf")
    #plt.show()


    # Stacked Grouped plot

    df_r = df1[['Type','#is_readopt',' t_e_read', ' t_n_read', ' t_e_insert', ' t_n_insert', "S"]]
    df_adj_d = df_r[(df_r["Type"] =="adj_d") &(df_r["#is_readopt"] == "no")]
    df_adj_rd = df_r[(df_r["Type"] =="adj_rd") &(df_r["#is_readopt"] == "yes")]
    df_ekey_d = df_r[(df_r["Type"] =="ekey_d") &(df_r["#is_readopt"] == "no")]
    df_ekey_rd = df_r[(df_r["Type"] =="ekey_rd") &(df_r["#is_readopt"] == "yes")]
    df_std_d = df_r[(df_r["Type"] =="std_d") &(df_r["#is_readopt"] == "no")]
    df_std_rd = df_r[(df_r["Type"] =="std_rd") &(df_r["#is_readopt"] == "yes")]

    print(df_adj_d.head())

    ax = df_adj_d[[' t_e_read', ' t_n_read', ' t_e_insert', ' t_n_insert']].plot.bar(stacked=True, position=0,
                                           width=.2, color=['orange', 'red', 'green', 'blue'])
    df_adj_rd[[' t_e_read', ' t_n_read', ' t_e_insert', ' t_n_insert']].plot.bar(ax=ax, stacked=True, position=1,
                                            width=.2, color=['orange', 'red', 'green','blue'])
    df_ekey_d[[' t_e_read', ' t_n_read', ' t_e_insert', ' t_n_insert']].plot.bar(ax=ax, stacked=True, position=2,
                                            width=.2, color=['orange', 'red', 'green','blue'])
    df_ekey_rd[[' t_e_read', ' t_n_read', ' t_e_insert', ' t_n_insert']].plot.bar(ax=ax, stacked=True, position=3,
                                            width=.2, color=['orange', 'red', 'green','blue'])
    df_std_d[[' t_e_read', ' t_n_read', ' t_e_insert', ' t_n_insert']].plot.bar(ax=ax, stacked=True, position=4,
                                            width=.2, color=['orange', 'red', 'green','blue'])
    df_std_rd[[' t_e_read', ' t_n_read', ' t_e_insert', ' t_n_insert']].plot.bar(ax=ax, stacked=True, position=5,
                                            width=.2, color=['orange', 'red', 'green','blue'])

    plt.xlabel("S Value", fontsize=18)
    plt.ylabel("Time (" r"$\mu$" "sec)", fontsize=18)
    plt.tight_layout()
    plt.savefig("./read_format_insert_time_stacked_grouped_barplot.pdf")
    plt.show()
    """
    print(list(df1))
    ax = df1[[' t_e_read', ' t_n_read', ' t_e_insert', ' t_n_insert']].plot.bar(stacked=True, position=1,
                                           width=.2, color=['orange', 'red', 'green', 'blue'])
    df1[[' t_e_read', ' t_n_read', ' t_e_insert', ' t_n_insert']].plot.bar(ax=ax, stacked=True, position=0,
                                                                           width=.2,
                                                                           color=['black', 'red', 'green', 'black'])
    plt.show()
    """

    """
    df = pd.DataFrame(dict(Subsidy=[3, 3, 3,3,3,3],
                           Bonus=[1, 1, 1,2,3,1],
                           Expense=[2, 2, 2,3,1,2],
                           Type=["A","A","B","C","C","C"]))
                      #,
                      #list('AABCCC'))

    print(df)
    ax = df[['Subsidy', 'Bonus']].plot.bar(stacked=True, position=1,
                                           width=.2, ylim=[0, 8], color=['orange', 'red'])
    df[['Expense']].plot.bar(ax=ax, position=0, width=.2, color=['green'])
    plt.show()
    """

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