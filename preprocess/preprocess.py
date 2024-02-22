import argparse
import os
import sys
import time
from datetime import datetime
from subprocess import check_output
from paths import ConfigReader


class Preprocess:
    def __init__(self, config_data: dict[str, any]):
        print("Preprocessing the graph")
        self.config_data = config_data
        self.cmdcnt = 0
        self.log_file = os.path.join(config_data['log_dir'], "insert.log")
        self.logger = open(self.log_file, "w")
        self.date = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        log_entry = "Path:" + \
                    self.config_data['graph_path'] + \
            "\nDate: " + self.date + "\n"
        if ('scale' in self.config_data):
            log_entry += "Scale: " + str(self.config_data['scale']) + "\n"
        self.logger.write(log_entry)
        # dump the config data
        self.logger.write("Config data:\n")
        for key, value in self.config_data.items():
            self.logger.write(key + ":" + str(value) + "\n")

    def log(self, message: str):
        self.logger.write(f"\n###Step{self.cmdcnt}####\n" + message + "\n")
        self.cmdcnt += 1

    def build_bulk_cmd(self):
        bulk_binary = "bulk_insert_low_mem"
        cmd = f"{self.config_data['cmd_root']}/preprocess/{bulk_binary} -d {self.config_data['dataset_name']} -e {self.config_data['num_edges']} -n {self.config_data['num_nodes']} -f {self.config_data['output_dir']}/{self.config_data['dataset_name']} -p {self.config_data['db_dir']} -l {self.config_data['log_dir']}/{bulk_binary}.log"
        return cmd

    def build_index_cmd(self, graph_type: str, is_ro: bool):
        db_name = f"{graph_type}_" + (
            "rd" if self.config_data['read_optimized'] else "d") + f"_{self.config_data['dataset_name']}"

        cmd = f"{self.config_data['cmd_root']}/preprocess/init_db -l {self.config_data['log_dir']}"
        cmd += f" -m {db_name} -p {self.config_data['db_dir']}"
        cmd += f" -s {self.config_data['dataset_name']} -d -g {graph_type} -x"
        if is_ro:
            cmd += " -r"
        return cmd

    def init_db(self, graph_type: str):
        db_name = f"{graph_type}_" + (
            "rd" if self.config_data['read_optimized'] else "d") + f"_{self.config_data['dataset_name']}"
        cmd = (
            f"{self.config_data['cmd_root']}/preprocess/init_db -n -m {db_name} -p {self.config_data['db_dir']} -s {self.config_data['dataset_name']} -o -d -g {graph_type} -e -l {self.config_data['log_dir']}")
        if self.config_data['read_optimized']:
            cmd += " -r"
        if self.config_data['weighted']:
            cmd += " -w"

        self.log(f"Initializing DB: {cmd}\n")
        if not self.config_data['dry_run']:
            os.system(cmd)

    def bulk_insert(self):
        self.log("Bulk Inserting graph")
        bulk_cmd = self.build_bulk_cmd()

        self.log(f"Command: {bulk_cmd}\n")
        start = time.perf_counter()
        if not self.config_data['dry_run']:
            os.system(bulk_cmd)
        stop = time.perf_counter()
        self.log(f"Time: {stop - start}\n")

    def create_index(self):
        self.log("Creating indices")
        for graph_type in ["std", "ekey"]:
            for is_ro in [True, False]:
                index_cmd = self.build_index_cmd(graph_type, is_ro)
                self.log(f"Creating index: {index_cmd}\n")
                if (not self.config_data['dry_run']):
                    os.system(index_cmd)

    def dump_config(self):
        # dump the config data
        for key, value in self.config_data.items():
            print(key + ":" + str(value) + "\n")

    def preprocess(self):
        ##############################
        # sort the graph
        ##############################

        self.log("Sorting the graph")
        sort_cmd = f"sort -g -k1,1 -k2,2 --parallel=10 -S 10G {self.config_data['graph_path']} > {self.config_data['output_dir']}/{self.config_data['dataset_name']}_sorted"

        self.config_data['sorted_graph'] = f"{self.config_data['output_dir']}/{self.config_data['dataset_name']}_sorted"
        self.log(f"Running sort command: {sort_cmd}\n")
        if (not self.config_data['dry_run']):
            st = time.time()
            os.system(sort_cmd)
            et = time.time()
            print(f"Time taken to sort the graph: {et - st}\n")
        ##############################
        # Split the graph into NUM_THREADS files
        ##############################
        self.log(
            f"Splitting the graph into NUM_THREAD({self.config_data['num_threads']}) files")
        split_cmd = f"split --number=l/{self.config_data['num_threads']} {self.config_data['sorted_graph']} {self.config_data['output_dir']}/{self.config_data['dataset_name']}_"
        self.log(f"Running split command: {split_cmd}\n")
        if (not self.config_data['dry_run']):
            st = time.time()
            os.system(split_cmd)
            et = time.time()
            print(f"Time taken to split the graph: {et - st}\n")

        ##############################
        # determine num_edges from the graph
        ##############################
        self.log(
            "Get the number of edges from the graph using awk")
        cmd = f"parallel awk -f count.awk ::: {self.config_data['output_dir']}/{self.config_data['dataset_name']}_a*" + \
              "| awk '{sum += $1} END {print sum}'"

        self.log(f"Running command: {cmd}\n")
        if (not self.config_data['dry_run']):
            st = time.time()
            found_edges = int(check_output(cmd, shell=True).split()[0])
            et = time.time()
            print(f"Time taken to count the edges: {et - st}\n")
            self.config_data['num_edges'] = found_edges
            self.log(f"The graph has {found_edges} edges")
            print("Found edges: " + str(found_edges))

        ##############################
        # compute num_nodes from the graph
        ##############################
        self.log("Constructing the nodes file")
        cmd = f"ls {self.config_data['output_dir']}/{self.config_data['dataset_name']}_a* | parallel awk -f nodes.awk " + \
            "{}" + \
            f" | sort -u -n >  {self.config_data['output_dir']}/{self.config_data['dataset_name']}_nodes"
        print(cmd)
        self.log(f"Running command: {cmd}\n")
        if (not self.config_data['dry_run']):
            st = time.time()
            os.system(cmd)
            et = time.time()
            print(f"Time taken to construct the nodes file: {et - st}\n")
        st = time.time()
        found_nodes = int(check_output(
            ["wc", "-l", f"{self.config_data['output_dir']}/{self.config_data['dataset_name']}_nodes"]).split()[0])
        et = time.time()
        print(f"Time taken to count the nodes: {et - st}\n")
        self.config_data['num_nodes'] = found_nodes
        self.log(f"Counting the number of nodes {found_nodes}")
        print("Found nodes: " + str(found_nodes))

        ##################################
        # reverse the graph
        ##################################
        self.log("Reversing the graph")
        reverse_cmd = f"awk '{{print $2\"\\t\"$1}}' {self.config_data['graph_path']} > {self.config_data['output_dir']}/{self.config_data['dataset_name']}_reverse"
        print(reverse_cmd)

        self.log(
            f"Generating the reverse graph (dst, src): {reverse_cmd}\n")
        if not self.config_data['dry_run']:
            st = time.time()
            os.system(reverse_cmd)
            et = time.time()
            print(f"Time taken to reverse the graph: {et - st}\n")

        sort_cmd = f"sort -g -k1,1 -k2,2 --parallel=10 -S 10G {self.config_data['output_dir']}/{self.config_data['dataset_name']}_reverse > {self.config_data['output_dir']}/{self.config_data['dataset_name']}_reverse_sorted"
        self.log(f"Running sort command: {sort_cmd}\n")
        if not self.config_data['dry_run']:
            st = time.time()
            os.system(sort_cmd)
            et = time.time()
            print(f"Time taken to sort the reverse graph: {et - st}\n")

        rename_cmd = f"mv {self.config_data['output_dir']}/{self.config_data['dataset_name']}_reverse_sorted {self.config_data['output_dir']}/{self.config_data['dataset_name']}_reverse"
        self.log(f"Renaming reverse graph: {rename_cmd}\n")
        if (not self.config_data['dry_run']):
            os.system(rename_cmd)

        self.config_data[
            'reverse_graph'] = f"{self.config_data['output_dir']}/{self.config_data['dataset_name']}_reverse"

        ##############################
        # Split the reverse graph into NUM_THREADS files
        ##############################
        self.log(
            f"Splitting the graph into NUM_THREAD({self.config_data['num_threads']}) files")
        split_cmd = f"split --number=l/{self.config_data['num_threads']} {self.config_data['reverse_graph']} {self.config_data['output_dir']}/{self.config_data['dataset_name']}_reverse_"
        self.log(f"Running split command: {split_cmd}\n")
        if (not self.config_data['dry_run']):
            st = time.time()
            os.system(split_cmd)
            et = time.time()
            print(f"Time taken to split the reverse graph: {et - st}\n")

        ##############################
        # Now construct the adjacency list files
        ##############################
        self.log("Constructing the adjacency list files")
        graph_type = "adj"  # not needed really, but including because getopts expects it
        cmd = f"{self.config_data['cmd_root']}/preprocess/mk_adjlists -d {self.config_data['dataset_name']} -e {self.config_data['num_edges']} -n {self.config_data['num_nodes']} -f {self.config_data['output_dir']}/{self.config_data['dataset_name']} -t {graph_type} -p {self.config_data['db_dir']} -l {self.config_data['log_dir']}/{graph_type}_rd_{self.config_data['dataset_name']}.log"
        self.log(f"Running command: {cmd}\n")
        if (not self.config_data['dry_run']):
            st = time.time()
            os.system(cmd)
            et = time.time()
            print(
                f"Time taken to construct the adjacency list files: {et - st}\n")

        self.log("Preprocessing complete")

    def cleanup(self):
        # remove all the intermediate files
        self.log("Cleaning up")
        cmd = f"rm {self.config_data['output_dir']}/{self.config_data['dataset_name']}_a*"
        self.log(f"Running command: {cmd}\n")
        if not self.config_data['dry_run']:
            os.system(cmd)
        cmd = f"rm {self.config_data['output_dir']}/{self.config_data['dataset_name']}_reverse_a*"
        self.log(f"Running command: {cmd}\n")
        if not self.config_data['dry_run']:
            os.system(cmd)
        cmd = f"rm {self.config_data['output_dir']}/{self.config_data['dataset_name']}_sorted"
        self.log(f"Running command: {cmd}\n")
        if not self.config_data['dry_run']:
            os.system(cmd)
        cmd = f"rm {self.config_data['output_dir']}/{self.config_data['dataset_name']}_reverse"
        self.log(f"Running command: {cmd}\n")
        if not self.config_data['dry_run']:
            os.system(cmd)

    def api_insert(self, graph_type):
        pass


def main():
    parser = argparse.ArgumentParser(
        description="Preprocess the graph and insert into the database")
    parser.add_argument("-l", "--log_dir", type=str, help="log directory")
    parser.add_argument("-g", "--graph_path", type=str, help="graph path")
    parser.add_argument("-v", "--num_nodes", type=int, help="number of nodes")
    parser.add_argument("-e", "--num_edges", type=int, help="number of edges")
    parser.add_argument("-d", "--db_dir", type=str, help="DB directory")
    parser.add_argument("-p", "--preprocess", action='store_true',
                        default=False, help="preprocess graphs")
    parser.add_argument("-x", "--index", action='store_true',
                        default=False, help="create index")
    parser.add_argument("-i", "--insert", action='store_true',
                        default=False, help="insert the graph. Bulk insert if -b is specified")
    parser.add_argument("-b", "--bulk_insert", action='store_true',
                        default=False, help="bulk insert")
    parser.add_argument("-m", "--num_threads", type=int,
                        help="number of threads", default=16)
    parser.add_argument("-a", "--num_partitions", type=int,
                        help="number of partitions", default=16)
    parser.add_argument("-s", "--dry_run", action='store_true', default=False,
                        help="dry run")
    parser.add_argument("-o", "--read_optimized", action='store_true', default=True,
                        help="read optimized")
    parser.add_argument("-c", "--cleanup", action='store_true', default=False,
                        help="cleanup any intermediate files, default is False")
    parser.add_argument("-w", "--weighted", action='store_true',
                        default=False, help="weighted graph")

    # check that there are some arguments passed
    assert len(sys.argv) > 1, "No arguments passed"
    args = parser.parse_args()

    # check that the graph path is passed
    assert args.graph_path is not None, "Graph path not specified"

    # check that the graph path exists
    assert os.path.exists(args.graph_path), "Graph path does not exist"
    args.dataset_name = args.graph_path.split("/")[-1].split(".")[0]

    # output the graphs in the same directory as the graph
    args.output_dir = os.path.dirname(args.graph_path)
    args.output_dir = os.path.join(args.output_dir, "preprocess")
    if args.output_dir == "":
        exit(f"Graph path {args.output_dir} is not valid")
    os.makedirs(args.output_dir, exist_ok=True)

    if (args.db_dir is None):
        print("Output directory not specified, using dataset path")
        args.db_dir = args.graph_path.split("/")[-1].split(".")[0]
    os.makedirs(args.db_dir, exist_ok=True)

    if args.log_dir is None:
        print("Using CWD as log directory")
        args.log_dir = os.getcwd()
    os.makedirs(args.log_dir, exist_ok=True)

    # read the config file
    config_data = ConfigReader("config.json").read_config()
    config_data.update(vars(args))  # add args to config

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

    preprocess = Preprocess(config_data)
    for graph_type in ["std", "ekey", "adj"]:
        preprocess.init_db(graph_type)

    if config_data['preprocess']:
        time_beg = time.time()
        preprocess.preprocess()
        time_end = time.time()
        print(f"Time taken to preprocess: {time_end - time_beg}\n")
        if config_data['cleanup']:
            preprocess.cleanup()

    if config_data['bulk_insert']:
        print("Bulk inserting")
        preprocess.bulk_insert()

    if config_data['index']:
        preprocess.create_index()


if __name__ == "__main__":
    main()
