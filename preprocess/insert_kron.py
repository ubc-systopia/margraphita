from pyexpat import ParserCreate
import os
import sys
import argparse
from preprocess import Preprocess
from paths import ConfigReader
from paths import GraphDatasetReader
import time


def api_insert(config: dict[str, any]):
    pass


def main():
    parser = argparse.ArgumentParser(
        description="Insert kron graphs into the database")
    parser.add_argument("--kron", action='store_true', default=False,
                        help="insert kron graphs")
    parser.add_argument(
        "--real", help="insert real-world graphs from graph_datasets.json; Default is False", action='store_true', default=False)
    parser.add_argument("--log_dir", type=str,
                        help="log directory. Defaults to CWD")
    parser.add_argument("-b", "--bulk", action='store_true',
                        default=False, help="Bulk insert graphs. Defaults to False")
    parser.add_argument("-x", "--index", action='store_true',
                        default=False, help="Create index. Defaults to False")
    parser.add_argument("--use_api", action='store_true', default=False,
                        help="use transactional GraphAPI. Default is False")

    parser.add_argument("-p", "--preprocess", action='store_true', default=False,
                        help="preprocess graphs. Default is False")
    parser.add_argument("-m", "--num_threads", action='store_true', default=16,
                        help="Number of threads to use for bulk insertion. This is also the number of partitions that are created for bulk insertion. Default is 16")
    parser.add_argument("-s", "--dry_run", action='store_true', default=False,
                        help="Dry run. Do not perform any operations. Default is False")
    parser.add_argument("-o", "--read_optimized", action='store_true', default=True,
                        help="read optimized; default is True")
    parser.add_argument("-c", "--cleanup", action='store_true', default=False,
                        help="cleanup any intermediate files, default is False")
    parser.add_argument("-w", "--weighted", action='store_true',
                        default=False, help="weighted graph, default is False")

    if len(sys.argv) == 1:
        parser.print_help(sys.stderr)
        sys.exit(1)
    args = parser.parse_args()
    config_data = ConfigReader("config.json").read_config()
    # see if we are in debug/release/profile/stat mode
    cwd = os.getcwd()
    if "debug" in cwd:
        config_data['cmd_root'] = config_data['DEBUG_PATH']
    elif "release" in cwd:
        config_data['cmd_root'] = config_data['RELEASE_PATH']
    elif "profile" in cwd:
        config_data['cmd_root'] = config_data['PROFILE_PATH']
    elif "stats" in cwd:
        config_data['cmd_root'] = config_data['STATS_PATH']

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

    if args.kron:
        sf_list = [26, 28, 30]
        for scale in sf_list:
            kron_graph_config = config_data
            n_edges = 16 * pow(2, scale)
            n_nodes = pow(2, scale)
            dataset_name = "s" + str(scale) + "_e16"
            graph_directory = os.path.join(
                kron_graph_config['KRON_GRAPHS_PATH'], dataset_name)
            graph = os.path.join(
                kron_graph_config['KRON_GRAPHS_PATH'], dataset_name, f"graph_{dataset_name}")

            kron_graph_config.update(vars(args))  # add args to config
            kron_graph_config['dataset_name'] = dataset_name
            kron_graph_config['db_dir'] = os.path.join(
                kron_graph_config['DB_DIR'], dataset_name)
            kron_graph_config['graph_dir'] = graph_directory

            # output the graphs in the same directory as the graph
            kron_graph_config['output_dir'] = os.path.dirname(graph)
            kron_graph_config['output_dir'] = os.path.join(
                kron_graph_config['output_dir'], "preprocess")
            print("Output dir: " + kron_graph_config['output_dir'])
            os.makedirs(kron_graph_config['output_dir'], exist_ok=True)

            kron_graph_config['graph_path'] = graph
            kron_graph_config['num_nodes'] = n_nodes
            kron_graph_config['num_edges'] = n_edges
            kron_graph_config['scale'] = scale

            preprocess_obj = Preprocess(kron_graph_config)
            print("is bulk: " + str(args.bulk)
                  + " \nindex: " + str(args.index)
                  + " \npreprocess: " + str(args.preprocess))
            for graph_type in ["std", "ekey", "adj", "split_ekey"]:
                preprocess_obj.init_db(graph_type)
            if (args.preprocess):
                time_beg = time.time()
                preprocess_obj.preprocess()
                time_end = time.time()
                print(f"Time taken to preprocess: {time_end - time_beg}\n")
            if (args.bulk):
                preprocess_obj.bulk_insert()
            if (args.index):
                preprocess_obj.create_index()
            preprocess_obj.log(
                "Finished inserting " + dataset_name + "\n-------------------\n")
            # check to see if the golden image exists at /drives/hdd_main/golden_images.
            # if not, then copy it over
            golden_image = os.path.join(
                "/drives/hdd_main/golden_images", dataset_name)
            if not os.path.exists(golden_image):
                print("Copying golden image")
                os.makedirs(golden_image, exist_ok=True)
                os.system(
                    f"cp -r {kron_graph_config['db_dir']}/* {golden_image}")
            else:
                print("Golden image already exists")
            exit(0)  # done with kron graphs

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
        dataset['db_dir'] = os.path.join(
            config_data['DB_DIR'], dataset['dataset_name'])
        if not os.path.exists(dataset['output_dir']):
            os.makedirs(dataset['output_dir'])
        dataset['scale'] = 0

        # dump the config struct to a tmp file
        with open("gotconfig.json", "w") as f:
            f.write(str(dataset))

        preprocess_obj = Preprocess(dataset)
        print("is bulk: " + str(args.bulk)
              + " \nindex: " + str(args.index)
              + " \npreprocess: " + str(args.preprocess))
        for graph_type in ["adj", "std", "ekey", "split_ekey"]:
            preprocess_obj.init_db(graph_type)
        if (args.preprocess):
            time_beg = time.time()
            preprocess_obj.preprocess()
            time_end = time.time()
            print(f"Time taken to preprocess: {time_end - time_beg}\n")

        if (args.bulk):
            preprocess_obj.bulk_insert()
        if (args.index):
            preprocess_obj.create_index()
        else:
            api_insert(config_data)
        preprocess_obj.log(
            "Finished inserting " + dataset['dataset_name'] + "\n-------------------\n")
        # check to see if the golden image exists at /drives/hdd_main/golden_images.
        # if not, then copy it over
        golden_image = os.path.join(
            "/drives/hdd_main/golden_images", dataset['dataset_name'])
        if not os.path.exists(golden_image):
            print("Copying golden image")
            os.makedirs(golden_image, exist_ok=True)
            os.system(
                f"cp -r {dataset['output_dir']}/* {golden_image}")
        else:
            print("Golden image already exists")


if __name__ == "__main__":
    main()
