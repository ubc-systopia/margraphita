# Author: Amee Trivedi
# Objective: Plot of the insert times for various values of nodes and edges.

import pandas as pd
import seaborn as sns
from matplotlib import pyplot as plt
import numpy as np

# Globals
df = pd.DataFrame()

def populate_df():
    global df
    s_range = np.arange(10,28)
    type_range = ["Std:RO", "Std:Non-RO", "Adj_List:RO", "Adj_List:Non-RO", "Edge_Key:RO", "Edge_Key:Non-RO"]

    s_list = np.repeat(s_range,len(type_range)).tolist()
    #print(s_list)

    type_list = type_range*len(s_range)
    #print(type_list)

    node_list = []
    edge_list = []

    for item in s_list:
        temp = pow(2,item)
        e=8
        node_list.append(temp)
        edge_list.append(e*temp)

    # cmd used to extract the values: grep "edges inserted in"  UUBC_Graphs/insert_time_txt | awk '{print $(NF)}'
    edge_time_list = [5220, 5247,
    9083,
    9923,
    8137,
    5177,
    9635,
    14300,
    17335,
    9328,
    9906,
    9804,
    20822,
    24715,
    20051,
    18814,
    22686,
    22309,
    40976,
    39773,
    38020,
    36585,
    36974,
    41353,
    78435,
    74398,
    75958,
    74925,
    75432,
    79949,
    150922,
    247223,
    150141,
    148692,
    149418,
    151074,
    293576,
    630143,
    324922,
    299285,
    297457,
    294909,
    669514,
    599886,
    617111,
    585695,
    587885,
    624701,
    1170586,
    1243130,
    1277864,
    1204616,
    1173970,
    1231751,
    10783942,
    2365977,
    2401118,
    2426337,
    2414351,
    2448367,
    4548911,
    4664956,
    4712708,
    4739059,
    4765398,
    4685495,
    9682881,
    9367129,
    9444309,
    9589397,
    9318318,
    9232225,
    18667942,
    35867089,
    19392204,
    19099873,
    18491314,
    19773664,
    37391644,
    49520212,
    37590820,
    39653770,
    37725498,
    38532357,
    75165748,
    77608385,
    75631852,
    76221427,
    76398471,
    80413024,
    155394463,
    160305183,
    150974220,
    155282622,
    156295666,
    156908705,
    308952385,
    314035807,
    309499764,
    324618138,
    313943301,
    313636494,
    623833951,
    631585579,
    618955831,
    624770730,
    625159152,
    629084499]

    # cmd used to extract the values: grep "nodes inserted in"  UUBC_Graphs/insert_time_txt | awk '{print $(NF)}'
    node_time_list = [740,
    745,
    1680,
    1784,
    758,
    582,
    1118,
    1036,
    4678,
    2696,
    1009,
    824,
    1480,
    10857,
    4614,
    4515,
    1706,
    1450,
    2533,
    2428,
    8616,
    8746,
    4124,
    3740,
    3896,
    3747,
    15589,
    16184,
    4775,
    5528,
    15135,
    7637,
    37362,
    36401,
    10366,
    7155,
    16677,
    344595,
    315733,
    72738,
    17785,
    15874,
    26126,
    24646,
    152252,
    154564,
    34484,
    33161,
    48449,
    47484,
    254636,
    277802,
    58497,
    57969,
    93816,
    126545,
    579589,
    590402,
    118352,
    101841,
    160652,
    172562,
    1170181,
    1157749,
    220134,
    189042,
    333341,
    309409,
    2409694,
    2424823,
    446344,
    374270,
    604202,
    596884,
    5476477,
    5458245,
    845040,
    748955,
    1155515,
    1137475,
    14531742,
    14935575,
    1601683,
    1428122,
    2234551,
    2279001,
    32492298,
    32239451,
    3253395,
    2973203,
    4535000,
    4581068,
    70116003,
    68244929,
    6770277,
    5619194,
    8531200,
    8908072,
    182302482,
    183273989,
    12762900,
    11032060,
    17035577,
    16380034,
    516888964,
    537459553,
    24052577,
    20491069]

    df = pd.DataFrame(list(zip(s_list, type_list, node_time_list, edge_time_list, node_list, edge_list)),
                      columns=["S", "Type", "Node_Time", "Edge_Time", "Node", "Edges"])
    print(df)
    return(0)

def plot():
    global df
    sns.set_style("whitegrid")

    # Draw a nested barplot by species and sex
    g = sns.catplot(
        data=df, kind="bar",
        x="S", y="Node_Time", hue="Type",
        ci="sd", palette="colorblind", alpha=.6, height=6, legend=False)
    #g.despine(left=True)
    plt.xlabel("S Value", fontsize=20)
    plt.ylabel("Time (" r"$\mu$" "sec)", fontsize=20)
    plt.legend(loc="upper left")
    plt.tight_layout()
    plt.savefig("./node_insert_time.pdf")
    plt.show()

    g = sns.catplot(
        data=df, kind="bar",
        x="S", y="Edge_Time", hue="Type",
        ci="sd", palette="colorblind", alpha=.6, height=6, legend=False)
    #g.despine(left=True)
    plt.xlabel("S Value", fontsize=20)
    plt.ylabel("Time (" r"$\mu$" "sec)", fontsize=20)
    plt.legend(loc="upper left")
    plt.tight_layout()
    plt.savefig("./edge_insert_time.pdf")
    plt.show()
    return(0)

def main():
    global df
    populate_df()
    plot()
    return(0)

if __name__ == "__main__":
    main()