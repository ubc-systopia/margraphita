import argparse
import os
import time
from datetime import datetime
from subprocess import check_output


class Preprocess:
    def __init__(self, config_data: dict[str, any]):
        self.config_data = config_data
        self.log_file = os.path.join(config_data['log_dir'], "insert_kron.log")
        self.log = open(self.log_file, "w")
        self.date = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        self.log_entry = "Scale: " + \
                         str(self.config_data['scale']) + "\nPath:" + self.config_data[
                             'graph_path'] + "\nDate: " + self.date + "\n"
        self.log.write(self.log_entry)

    def build_bulk_cmd(self, graph_type: str, is_ro: bool):
        cmd = f"{self.config_data['RELEASE_PATH']}/preprocess/bulk_insert -d {self.config_data['dataset_name']} -e {self.config_data['num_edges']} -n {self.config_data['num_nodes']} -f {self.config_data['graph_path']} -t {graph_type} -p {self.config_data['output_dir']} -l {self.config_data['log_dir']}/{graph_type}_rd_{self.config_data['dataset_name']}.log "
        if is_ro:
            cmd += " -r"
        return cmd

    def build_index_cmd(self, graph_type: str, is_ro: bool): 
        cmd = f"{self.config_data['RELEASE_PATH']}/preprocess/init_db -b PR -f {self.config_data['log_dir']}"
        cmd += f" -m {graph_type}_" + (
            "rd" if is_ro else "d") + f"_{self.config_data['dataset_name']} -a {self.config_data['output_dir']}"
        cmd += f" -s {self.config_data['dataset_name']} -d -l {graph_type} -x"
        if is_ro:
            cmd += " -r"
        return cmd
#-f is for log_dir and stats_dir
    def build_init_cmd(self, graph_type: str, is_ro: bool):
        cmd = f"{self.config_data['RELEASE_PATH']}/preprocess/init_db -n -m {graph_type}_" + (
            "rd" if is_ro else "d") + f"_{self.config_data['dataset_name']} -a {self.config_data['output_dir']} -s {self.config_data['dataset_name']} -o -d -l {graph_type} -e -f {self.config_data['log_dir']}"
        if is_ro:
            cmd += " -r"
        return cmd

    def bulk_insert(self):
        for graph_type in ['std', 'ekey', 'adj', 'elist']:
            for is_ro in [True]:
                # directed, read_optimized, init DB
                init_cmd = self.build_init_cmd(graph_type, is_ro)
                self.log.write(
                    f"Initializing DB, read_optimize= {is_ro}: {init_cmd}\n")
                os.system(init_cmd)

                # directed, read_optimized, bulk insert
                name_str = f"{graph_type}_" + \
                    ("rd" if is_ro else "d") + \
                    f"_{self.config_data['dataset_name']}"
                self.log.write(f"Bulk Inserting {name_str}\n")
                bulk_cmd = self.build_bulk_cmd(graph_type, is_ro)

                self.log.write(f"Command: {bulk_cmd}\n")
                start = time.perf_counter()
                os.system(bulk_cmd)
                stop = time.perf_counter()
                self.log.write(f"Time: {stop - start}\n")

    def create_index(self):
        for graph_type in ["std", "ekey", "elist"]:
            for is_ro in [True]:
                index_cmd = self.build_index_cmd(graph_type, is_ro)
                self.log.write(f"Creating index: {index_cmd}\n")
                os.system(index_cmd)

    def preprocess(self):
        ##############################
        # assert num_edges matches the number of edges in the graph
        ##############################
        found_edges = int(check_output(
            ["wc", "-l", self.config_data['graph_path']]).split()[0])
        assert found_edges == self.config_data['num_edges']

        # sed the graph to replace tabs with newlines and compute number of nodes
        sed_cmd = f"sed 's/\\t/\\n/' {self.config_data['graph_path']} > {self.config_data['graph_path']}.tmp"
        print(sed_cmd)
        self.log.write(f"Running sed command: {sed_cmd}\n")
        os.system(sed_cmd)

        sort_cmd = f"sort -n -u {self.config_data['graph_path']}.tmp > {self.config_data['graph_path']}_nodes"
        self.log.write(f"Running sort command: {sort_cmd}\n")
        os.system(sort_cmd)
        found_nodes = int(check_output(
            ["wc", "-l", f"{self.config_data['graph_path']}_nodes"]).split()[0])
        self.config_data['num_nodes'] = found_nodes

        ##################################
        # reverse the graph
        ##################################
        reverse_cmd = f"awk '{{print $2\"\\t\"$1}}' {self.config_data['graph_path']} > {self.config_data['graph_path']}_reverse"
        print(reverse_cmd)
        self.log.write(
            f"Generating the reverse graph (dst, src): {reverse_cmd}\n")
        os.system(reverse_cmd)
        
        sort_cmd = f"sort -n -k 1,1 -k 2,2 {self.config_data['graph_path']}_reverse > {self.config_data['graph_path']}_reverse_sorted"
        self.log.write(f"Running sort command: {sort_cmd}\n")
        os.system(sort_cmd)

        rename_cmd = f"mv {self.config_data['graph_path']}_reverse_sorted {self.config_data['graph_path']}_reverse"
        self.log.write(f"Renaming reverse graph: {rename_cmd}\n")
        os.system(rename_cmd)

        remap_cmd = f"{self.config_data['RELEASE_PATH']}/preprocess/dense_vertexranges -n {found_nodes} -e {found_edges} -f {self.config_data['graph_path']}_reverse"
        self.log.write(f"Running remap command: {remap_cmd}\n")
        os.system(remap_cmd)

        ##################################
        # Construct the id map and construct the new graph based on these ID mappings
        ##################################
        remap_cmd = f"{self.config_data['RELEASE_PATH']}/preprocess/dense_vertexranges -n {found_nodes} -e {found_edges} -f {self.config_data['graph_path']} "
        self.log.write(f"Running remap command: {remap_cmd}\n")
        os.system(remap_cmd)

        # split the nodes into NUM_THREADS files
        split_cmd = f"split --number=l/{self.config_data['num_threads']} {self.config_data['graph_path']}_nodes {self.config_data['graph_path']}_nodes"
        self.log.write(f"Running split command: {split_cmd}\n")
        os.system(split_cmd)


def main():
    parser = argparse.ArgumentParser(
        description="Preprocess the graph and insert into the database")
    parser.add_argument("--log_dir", type=str, help="log directory")
    parser.add_argument("--dataset_name", type=str, help="dataset name")
    parser.add_argument("--graph_dir", type=str, help="graph directory")
    parser.add_argument("--graph_path", type=str, help="graph path")
    parser.add_argument("--num_nodes", type=int, help="number of nodes")
    parser.add_argument("--num_edges", type=int, help="number of edges")
    parser.add_argument("--graph_type", type=str, help="graph type")
    parser.add_argument("--output_dir", type=str, help="output directory")
    parser.add_argument("--preprocess", action='store_true',
                        default=True, help="preprocess graphs")
    parser.add_argument("--index", action='store_true',
                        default=False, help="create index")
    parser.add_argument("--insert", action='store_true',
                        default=True, help="bulk insert the graph")
    parser.add_argument("--num_threads", type=int,
                        help="number of threads", default=16)
    parser.add_argument("--num_partitions", type=int,
                        help="number of partitions", default=16)

    args = parser.parse_args()

    if args.log_dir is None:
        print("Using CWD as log directory")
        args.log_dir = os.getcwd()

    preprocess = Preprocess(vars(args))
    preprocess.preprocess()


if __name__ == "__main__":
    main()
