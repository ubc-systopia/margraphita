from pyexpat import ParserCreate
import subprocess
import os
import getopt
import sys
import argparse
from datetime import datetime
from preprocess import Preprocess
from paths import ConfigReader
import pprint


def api_insert(config: dict[str, any]):
    pass


def main():
    parser = argparse.ArgumentParser(
        description="Insert kron graphs into the database")
    parser.add_argument("--log_dir", type=str, help="log directory")
    parser.add_argument("--preprocess", action='store_true', default=True,
                        help="preprocess graphs")
    parser.add_argument("--bulk", action='store_true',
                        default=False, help="insert graphs")
    parser.add_argument("--index", action='store_true',
                        default=True, help="create index")
    parser.add_argument("--use_api", action='store_true', default=False,
                        help="use transactional GraphAPI")

    args = parser.parse_args()
    config_data = ConfigReader("config.json").read_config()

    if args.log_dir is None:
        print("Using CWD as log directory")
        args.log_dir = os.getcwd()

    if args.bulk and args.use_api:
        print("Cannot use both bulk and api insert")
        return

    # print arguments
    print("Log directory: " + args.log_dir)
    print("Preprocess: " + str(args.preprocess))
    print("Bulk: " + str(args.bulk))
    print("Index: " + str(args.index))
    print("Use API: " + str(args.use_api))

    for scale in range(10, 11):
        n_edges = 8*pow(2, scale)
        n_nodes = pow(2, scale)
        dataset_name = "s" + str(scale) + "_e8"
        graph_directory = os.path.join(
            config_data['KRON_GRAPHS_PATH'], dataset_name)
        graph = os.path.join(
            config_data['KRON_GRAPHS_PATH'], dataset_name, f"graph_{dataset_name}")

        config_data.update(vars(args))  # add args to config
        config_data['dataset_name'] = dataset_name
        config_data['graph_dir'] = graph_directory
        config_data['output_dir'] = os.path.join(
            config_data['DB_DIR'], dataset_name)
        config_data['graph_path'] = graph
        config_data['num_nodes'] = n_nodes
        config_data['num_edges'] = n_edges
        config_data['scale'] = scale
        print(config_data)

        preprocess_obj = Preprocess(config_data)
        if (args.preprocess):
            preprocess_obj.preprocess()
        if (args.bulk):
            preprocess_obj.bulk_insert()
        if (args.index):
            preprocess_obj.create_index()
        else:
            api_insert(config_data)
        preprocess_obj.log.write(
            "Finished inserting " + dataset_name + "\n-------------------\n")


if __name__ == "__main__":
    main()
