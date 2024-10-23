import argparse
import os
import subprocess
import time
from datetime import datetime
from subprocess import check_output
from paths import ConfigReader
import grp


class BenchmarkRunner:
    def __init__(self, config_data: dict[str, any]):
        self.datasets = ["road_asia"] #, "orkut", "road_usa", "livejournal", "dota_league", "graph500_22", "graph500_23", "graph500_26", "graph500_28", "graph500_30"]
        self.config_data = config_data
        # for scale in range(10, 11):
        #     self.datasets.append(f"s{scale}_e8")
        self.types = ["adj"] #["std", "adj", "ekey"]
        #make log directory if not present
        os.system(f"mkdir -p {config_data['LOG_DIR']}")
        self.log_file = config_data['LOG_DIR'] + "/benchmark_run.log"
        print(f"Log file: {self.log_file}")
        self.log_handle = open(f"{self.log_file}", "a+")
        self.log_handle.write(f"Starting benchmark run at {datetime.now()}\n")
    
    def log(self, msg: str):
        self.log_handle.write(msg + "\n")

    def make_pr_cmd(self, binary_name: str, ds: str, graph_type: str):
        cmd = f"{binary_name} -m {graph_type}_d_{ds} -g {graph_type} -p {self.config_data['DB_DIR']}/{ds} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else ""
        cmd+= f" >> {self.config_data['LOG_DIR']}/{ds}_pr_{graph_type}.log"
        return cmd

    def make_bfs_cmd(self, binary_name: str, ds: str, graph_type: str, vert: int):
        cmd = f"{binary_name} -m {graph_type}_d_{ds} -p {self.config_data['DB_DIR']}/{ds} -g {graph_type} -v {vert} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else ""
        cmd+= f">> {self.config_data['LOG_DIR']}/{ds}_bfs_{graph_type}.log"
        return cmd

    def make_tc_cmd(self, binary_name: str, ds: str, graph_type: str):
        cmd = f"{binary_name} -m {graph_type}_d_{ds} -p {self.config_data['DB_DIR']}/{ds} -g {graph_type} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else ""
        cmd+= f" >> {self.config_data['LOG_DIR']}/{ds}_tc_{graph_type}.log"
        return cmd

    def make_cc_cmd(self, binary_name: str, ds: str, graph_type: str, variant: str):
        cmd = f"{binary_name} -m {graph_type}_d_{ds} -p {self.config_data['DB_DIR']}/{ds} -g {graph_type} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else ""
        cmd+= f">> {self.config_data['LOG_DIR']}/{ds}_cc_{graph_type}.log"
        return cmd

    def make_sssp_cmd(self, binary_name: str, ds: str, graph_type: str, vert: int):
        cmd = f"{binary_name} -m {graph_type}_d_{ds} -p {self.config_data['DB_DIR']}/{ds} -g {graph_type} -v {vert} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else "" 
        cmd+= f" >> {self.config_data['LOG_DIR']}/{ds}_sssp_{graph_type}.log"
        return cmd

    def make_perf_params(self, benchmark: str, ds: str, graph_type: str):
        return f"perf record -e cycles,instructions,cache-references,cache-misses,branch-instructions,branch-misses,context-switches,cpu-migrations,page-faults,minor-faults,major-faults,alignment-faults,emulation-faults,ref-cycles -o {self.config_data['LOG_DIR']}/{benchmark}/{benchmark}_{ds}_{graph_type}_perf.data --call-graph fp"

    def pr(self):
        for ds in self.datasets:
            for graph_type in self.types:
                cmd = self.make_pr_cmd(
                    f"{self.config_data['RELEASE_PATH']}/benchmark/pr_vc", ds, graph_type)
                print(cmd)
                self.log(cmd)
                if(self.config_data['dry_run']):
                    continue
                os.system(cmd)

    def bfs(self):
        for ds in self.datasets:
            random_verts = None
            with open(f"{self.config_data['DB_DIR']}/{ds}/{ds}.bfsver", "r") as random_file:
                random_verts = random_file.readlines()
            for graph_type in self.types:
                for vert in random_verts:
                    vert = int(vert.strip())
                    cmd = self.make_bfs_cmd(f"{self.config_data['RELEASE_PATH']}/benchmark/bfs_parallel", ds, graph_type, vert)
                    self.log(cmd)
                    if(self.config_data['dry_run']):
                        continue
                    os.system(cmd)

    def tc(self):
        for ds in self.datasets:
            for graph_type in self.types:
                cmd = self.make_tc_cmd(
                    f"{self.config_data['RELEASE_PATH']}/benchmark/tc", ds, graph_type)
                self.log(cmd)
                if(self.config_data['dry_run']):
                    continue
                os.system(cmd)


    def cc(self):
        for ds in self.datasets:
            for graph_type in self.types:
                cmd = self.make_cc_cmd(
                    f"{self.config_data['RELEASE_PATH']}/benchmark/cc_parallel", ds, graph_type, "cc_parallel")
                self.log(cmd+"\n")
                if(self.config_data['dry_run']):
                    continue
                os.system(cmd)

    def sssp(self):
        for ds in self.datasets:
            random_verts = None
            with open(f"{self.config_data['DB_DIR']}/{ds}/{ds}.bfsver", "r") as random_file:
                random_verts = random_file.readlines()
            for graph_type in self.types:
                for vert in random_verts:
                    vert = int(vert.strip())
                    cmd = self.make_sssp_cmd(
                        f"{self.config_data['RELEASE_PATH']}/benchmark/sssp_parallel", ds, graph_type, vert)
                    self.log(cmd)
                    if(self.config_data['dry_run']):
                        continue
                    os.system(cmd)


def main():

    # # Set the group ID to 'graphs'
    # group_id = grp.getgrnam("graphs").gr_gid
    # os.setgid(group_id)
    # assert (os.getgid() == group_id)

    parser = argparse.ArgumentParser(
        description="Run benchmarks"
    )
    parser.add_argument("--log_dir", type=str, help="log directory")
    parser.add_argument('-b','--benchmarks', nargs='*', help='Benchmarks to run', default=["pr", "bfs", "tc_iter", "cc", "sssp"])
    parser.add_argument("--dry_run", action='store_true',default=False, help="Dry run")

    args = parser.parse_args()
    config_data = ConfigReader("config.json").read_config()
    config_data['benchmarks'] = args.benchmarks
    config_data['dry_run'] = args.dry_run

    print(config_data['benchmarks'])

    if args.log_dir is None:
        print("Using CWD/logs as log directory")
        log_dir = os.path.join(os.getcwd(), "logs")
        os.system(f"mkdir -p {log_dir}")
        config_data['LOG_DIR'] = log_dir
    else:
        config_data['LOG_DIR'] = args.log_dir
    
    print(f"Log directory: {config_data['LOG_DIR']}")

    #get number of hardware threads available
    num_threads = int(subprocess.check_output("nproc").decode("utf-8").strip())
    if num_threads > 100:
        num_threads+=100 #idk really
        config_data['config_string'] = f"session_max={num_threads}"

    benchmark_runner = BenchmarkRunner(config_data)

    for benchmark in config_data['benchmarks']:
        if benchmark == "pr":
            print("Running PageRank")
            benchmark_runner.pr()
        elif benchmark == "bfs":
            benchmark_runner.bfs()
        elif benchmark == "tc":
            benchmark_runner.tc()
        elif benchmark == "cc":
            benchmark_runner.cc()
        elif benchmark == "sssp":
            benchmark_runner.sssp()
        else:
            print(f"Unknown benchmark {benchmark}")


if __name__ == "__main__":
    main()
