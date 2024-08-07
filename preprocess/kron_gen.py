import os
import sys
from argparse import ArgumentParser
from paths import ConfigReader


def main():
    config_data = ConfigReader("config.json").read_config()
    print(config_data)
    ef = 16
    for scale in [26, 28, 30]:
        n_edges = ef * pow(2, scale)
        n_nodes = pow(2, scale)
        dataset_name = "s" + str(scale) + f"_e{ef}"
        graph_directory = os.path.join(
            config_data['KRON_GRAPHS_PATH'], dataset_name)
        graph = os.path.join(graph_directory, f"graph_{dataset_name}")

        os.makedirs(graph_directory, exist_ok=True)
        print("Generating graph s" + str(scale) + f"_e{ef}")

        if "PaRMAT" in config_data['KRON_GEN_PATH']:
            gen_cmd = f"{config_data['KRON_GEN_PATH']} -nEdges {n_edges} -nVertices {n_nodes} -noEdgeToSelf -noDuplicateEdges -sorted -output {graph}"
            print(gen_cmd)
            os.system(gen_cmd)
        elif "smooth_kron_gen" in config_data['KRON_GEN_PATH']:
            gen_cmd = f"{config_data['KRON_GEN_PATH']} {n_edges} {n_nodes} 0 0 > -o {graph}"
            os.system(gen_cmd)
        elif "gapbs" in config_data['KRON_GEN_PATH']:
            gen_cmd = f"{config_data['KRON_GEN_PATH']} -g {scale} -k {ef} -e {graph}"
            print("Generating Kronecker Graph: " + gen_cmd)
            os.system(gen_cmd)
            # now generate uniform random graphs
            uniform_graph_dir = os.path.join(
                config_data['KRON_GRAPHS_PATH'], dataset_name + "_uniform")
            os.makedirs(uniform_graph_dir, exist_ok=True)
            uniform_graph = os.path.join(uniform_graph_dir, f"graph_{dataset_name}_uniform")
            gen_cmd = f"{config_data['KRON_GEN_PATH']} -u {scale} -k {ef} -e {uniform_graph}"
            print("Generating uniform random graph: " + gen_cmd)
            os.system(gen_cmd)
        else:
            print("Unknown generator")
            sys.exit(1)

if __name__ == "__main__":
    main()
