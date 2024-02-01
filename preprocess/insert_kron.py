from pyexpat import ParserCreate
import subprocess
import os
import getopt
import sys
import argparse
from datetime import datetime
from preprocess import Preprocess
from paths import ConfigReader
from paths import GraphDatasetReader
import pprint
import grp


def api_insert(config: dict[str, any]):
    pass


def main():
    parser = argparse.ArgumentParser(
        description="Insert kron graphs into the database")
    parser.add_argument("--log_dir", type=str, help="log directory")
    parser.add_argument("--preprocess", action='store_true', default=False,
                        help="preprocess graphs")
    parser.add_argument("--bulk", action='store_true',
                        default=False, help="insert graphs")
    parser.add_argument("--index", action='store_true',
                        default=False, help="create index")
    parser.add_argument("--use_api", action='store_true', default=False,
                        help="use transactional GraphAPI")
    parser.add_argument("--low_mem", action='store_true', default=False,
                        help="use low memory mode")

    args = parser.parse_args()
    config_data = ConfigReader("config.json").read_config()

    if args.log_dir is None:
        if "LOG_DIR" in config_data:
            args.log_dir = config_data['LOG_DIR']
            if not os.path.exists(args.log_dir):
                os.mkdir(args.log_dir)
        else:
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
        kron_graph_config = config_data
        n_edges = 8*pow(2, scale)
        n_nodes = pow(2, scale)
        dataset_name = "s" + str(scale) + "_e8"
        graph_directory = os.path.join(
            kron_graph_config['KRON_GRAPHS_PATH'], dataset_name)
        graph = os.path.join(
            kron_graph_config['KRON_GRAPHS_PATH'], dataset_name, f"graph_{dataset_name}")

        kron_graph_config.update(vars(args))  # add args to config
        kron_graph_config['dataset_name'] = dataset_name
        kron_graph_config['graph_dir'] = graph_directory
        kron_graph_config['output_dir'] = os.path.join(
            kron_graph_config['DB_DIR'], dataset_name)
        kron_graph_config['graph_path'] = graph
        kron_graph_config['num_nodes'] = n_nodes
        kron_graph_config['num_edges'] = n_edges
        kron_graph_config['scale'] = scale

        preprocess_obj = Preprocess(kron_graph_config)
        print("is bulk: " + str(args.bulk)
              + " \nindex: " + str(args.index)
                + " \npreprocess: " + str(args.preprocess))
        if (args.preprocess):
            preprocess_obj.preprocess()
        else:
            preprocess_obj.config_data['graph_path'] += "_sorted"
            #preprocess step is skipped, so we need to add _sorted to the graph path
        if (args.bulk):
            preprocess_obj.bulk_insert()
        if (args.index):
            preprocess_obj.create_index()
        else:
            api_insert(config_data)
        preprocess_obj.log.write(
            "Finished inserting " + dataset_name + "\n-------------------\n")

    # To insert graphs that are not Kronecker graphs
    # This can be read from a DB, but for now we just read from a file
    dataset_json = GraphDatasetReader("graph_datasets.json").read_config()
    for dataset in dataset_json:
        dataset.update(config_data)  # add config to dataset
        dataset["graph_dir"] = os.path.dirname(dataset["graph_path"])
        dataset.update(vars(args))  # add args to config
        graph = dataset['graph_path']
        dataset['output_dir'] = os.path.join(
            config_data['DB_DIR'], dataset['dataset_name'])
        dataset['scale'] = 0

        preprocess_obj = Preprocess(dataset)
        print("is bulk: " + str(args.bulk)
              + " \nindex: " + str(args.index)
                + " \npreprocess: " + str(args.preprocess))
        if (args.preprocess):
            preprocess_obj.preprocess()
        if (args.bulk):
            preprocess_obj.bulk_insert()
        if (args.index):
            preprocess_obj.create_index()
        else:
            api_insert(config_data)
        preprocess_obj.log.write(
            "Finished inserting " + dataset['dataset_name'] + "\n-------------------\n")


if __name__ == "__main__":
    main()
