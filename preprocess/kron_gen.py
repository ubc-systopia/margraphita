import os
import sys
from argparse import ArgumentParser
from paths import ConfigReader


def main():
    config_data = ConfigReader("config.json").read_config()
    print(config_data)
    for ef in [8]:
        for scale in range(10, 16):
            n_edges = ef * pow(2, scale)
            n_nodes = pow(2, scale)
            dataset_name = "s" + str(scale) + f"_e{ef}"
            graph_directory = os.path.join(config_data['KRON_GRAPHS_PATH'], dataset_name)
            graph = os.path.join(graph_directory, f"graph_{dataset_name}")
            os.makedirs(graph_directory, exist_ok=True)
            print("Generating graph s" + str(scale) + f"_e{ef}")

            if "PaRMAT" in config_data['KRON_GEN_PATH']:
                gen_cmd = f"{config_data['KRON_GEN_PATH']} -nEdges {n_edges} -nVertices {n_nodes} -noEdgeToSelf -noDuplicateEdges -sorted -output {graph}"
                print(gen_cmd)
                os.system(gen_cmd)
            else:
                gen_cmd = f"{config_data['KRON_GEN_PATH']} {n_edges} {n_nodes} 0 0 > -o {graph}"
                os.system(gen_cmd)


if __name__ == "__main__":
    main()
