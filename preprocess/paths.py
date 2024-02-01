import json
import os
import subprocess


class ConfigReader:
    def __init__(self, config_file):
        self.config_file = config_file
        self.config = None

    def read_config(self):
        config_data = {}
        with open(self.config_file, 'r') as file_handle:
            config_data = json.load(file_handle)

        try:
            project_dir = config_data['GRAPH_PROJECT_DIR']
            project_dir = os.path.join("/home", os.getlogin(), project_dir)
            print("Using GRAPH_PROJECT_DIR as set - ${GRAPH_PROJECT_DIR}")
        except KeyError:
            print("Please set GRAPH_PROJECT_DIR as the path to the margraphita repo in the build/config.json file.")
            exit(1)

        try:
            kron_gen_name = config_data['KRON_GEN_PATH']
            kron_gen_name = os.path.join("/home", os.getlogin(), kron_gen_name)
            if "PaRMAT" in kron_gen_name:
                print("Using PaRMAT as the Kronecker generator")
            elif "smooth_kron" in kron_gen_name:
                print("Using KronGen as the Kronecker generator")
            else:
                print(
                    "Please set KRON_GEN_PATH as either PaRMAT or KronGen in the build/config.json file.")
                exit(1)
        except KeyError:
            print(
                "Please set KRON_GEN_PATH as either PaRMAT or KronGen in the build/config.json file.")
            exit(1)

        try:
            kron_graphs_path = config_data['KRON_GRAPHS_PATH']
            print(f"Using KRON_GRAPHS_PATH as set - {kron_graphs_path}")
        except KeyError:
            print(
                "Please set KRON_GRAPHS_PATH as the path to the kron_gen repo in the build/config.json file.")
            exit(1)

        try:
            log_dir = config_data['LOG_DIR']
            git_hash_id = subprocess.check_output(
                ['git', 'rev-parse', '--short', 'HEAD']).strip().decode('ascii')
            print(f"The commit hash is {git_hash_id}")
            config_data['log_dir'] = os.path.join(log_dir, git_hash_id)
        except KeyError:
            print("no log directory found. will default to CWD")
            config_data['log_dir'] = os.path.join(os.getcwd(), git_hash_id)

        config_data['PROFILE_PATH'] = os.path.join(
            project_dir, "build", "profile")
        config_data['DEBUG_PATH'] = os.path.join(
            project_dir, "build", "debug")
        config_data['RELEASE_PATH'] = os.path.join(
            project_dir, "build", "release")
        config_data['STATS_PATH'] = os.path.join(
            project_dir, "build", "wt_stats")
        config_data['OUTPUT_PATH'] = os.path.join(project_dir, "outputs")
        return config_data


class GraphDatasetReader:
    def __init__(self, config_file):
        self.config_file = config_file
        self.config = None

    def read_config(self):
        config_data = {}
        with open(self.config_file, 'r') as file_handle:
            config_data = json.load(file_handle)

        config_ok = True
        for dataset in config_data:
            try:
                assert (os.path.exists(dataset['graph_path']))
            except AssertionError:
                print(
                    f"Graph file {dataset['graph_path']} not found. Please check the path.")
                config_ok = False
            except KeyError:
                print(f"Missing keys; Please check the config file.")
                config_ok = False

            try:
                dataset['num_nodes'] = int(dataset['num_nodes'])
                assert (dataset['num_nodes'] > 0)
            except KeyError:
                print(
                    f"Missing num_nodes for {dataset['dataset_name']}; Please check the config file.")
                config_ok = False
            except AssertionError:
                print(
                    f"Error while parsing {dataset['dataset_name']} : num_nodes must be a positive integer")
                config_ok = False

            try:
                dataset['num_edges'] = int(dataset['num_edges'])
                assert (dataset['num_edges'] > 0)
            except KeyError:
                print(
                    f"Missing num_edges for {dataset['dataset_name']}; Please check the config file.")
                config_ok = False
            except AssertionError:
                print(
                    f"Error while parsing {dataset['dataset_name']} : num_edges must be a positive integer")
                config_ok = False
        if (not config_ok):
            exit(1)
        else:
            return config_data
