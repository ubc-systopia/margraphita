import json
import os
import json


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
            print("Using GRAPH_PROJECT_DIR as set - ${GRAPH_PROJECT_DIR}")
        except KeyError:
            print("Please set GRAPH_PROJECT_DIR as the path to the margraphita repo in the build/config.json file.")
            exit(1)

        try:
            kron_gen_name = config_data['KRON_GEN_PATH']
            if "PaRMAT" in kron_gen_name:
                print("Using PaRMAT as the Kronecker generator")
            elif "smooth_kron" in kron_gen_name:
                print("Using KronGen as the Kronecker generator")
            else:
                print("Please set KRON_GEN_PATH as either PaRMAT or KronGen in the build/config.json file.")
                exit(1)
        except KeyError:
            print("Please set KRON_GEN_PATH as either PaRMAT or KronGen in the build/config.json file.")
            exit(1)

        try:
            kron_graphs_path = config_data['KRON_GRAPHS_PATH']
            print(f"Using KRON_GRAPHS_PATH as set - {kron_graphs_path}")
        except KeyError:
            print("Please set KRON_GRAPHS_PATH as the path to the kron_gen repo in the build/config.json file.")
            exit(1)

        config_data['PROFILE_PATH'] = os.path.join(project_dir, "build", "profile")
        config_data['RELEASE_PATH'] = os.path.join(project_dir, "build", "release")
        config_data['STATS_PATH'] = os.path.join(project_dir, "build", "wt_stats")
        config_data['OUTPUT_PATH'] = os.path.join(project_dir, "outputs")
        return config_data
