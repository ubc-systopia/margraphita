import argparse
import os
import subprocess
import time
from datetime import datetime
from subprocess import check_output
from paths import ConfigReader
import grp


class BenchmarkRunner:
    def __init__(self, config_data: dict[str, any], log_file: str):
        self.datasets = []
        self.config_data = config_data
        for scale in range(10, 11):
            self.datasets.append(f"s{scale}_e8")
        self.types = ["std", "adj", "ekey"]
        self.log_file = log_file

    def make_pr_cmd(self, binary_name: str, ds: str, graph_type: str):
        cmd = f"{binary_name} -m {graph_type}_rd_{ds} -b PR -a {self.config_data['DB_DIR']}/{ds} -s {ds} -f {self.config_data['LOG_DIR']}/pr -r -d -l {graph_type} -i 10 -t 0.0001"
        return cmd

    def make_bfs_cmd(self, binary_name: str, ds: str, graph_type: str):
        cmd = f"{binary_name} -m {graph_type}_rd_{ds} -b BFS -a {self.config_data['DB_DIR']}/{ds} -s {ds} -f {self.config_data['LOG_DIR']}/bfs -r -d -l {graph_type}"
        return cmd

    def make_tc_cmd(self, binary_name: str, ds: str, graph_type: str):
        cmd = f"{binary_name} -m {graph_type}_rd_{ds} -b TC -a {self.config_data['DB_DIR']}/{ds} -s {ds} -f {self.config_data['LOG_DIR']}/tc -r -d -l {graph_type}"
        return cmd

    def make_cc_cmd(self, binary_name: str, ds: str, graph_type: str, variant: str):
        cmd = f"{binary_name} -m {graph_type}_rd_{ds} -b CC -a {self.config_data['DB_DIR']}/{ds} -s {ds} -f {self.config_data['LOG_DIR']}/{variant} -r -d -l {graph_type}"
        return cmd

    def make_sssp_cmd(self, binary_name: str, ds: str, graph_type: str):
        cmd = f"{binary_name} -m {graph_type}_rd_{ds} -b SSSP -a {self.config_data['DB_DIR']}/{ds} -s {ds} -f {self.config_data['LOG_DIR']}/sssp -r -d -l {graph_type}"
        return cmd

    def make_perf_params(self, benchmark: str, ds: str, graph_type: str):
        return f"perf record -e cycles,instructions,cache-references,cache-misses,branch-instructions,branch-misses,context-switches,cpu-migrations,page-faults,minor-faults,major-faults,alignment-faults,emulation-faults,ref-cycles -o {self.config_data['LOG_DIR']}/{benchmark}/{benchmark}_{ds}_{graph_type}_perf.data --call-graph fp"

    def run_pr(self):
        os.system(f"mkdir -p {self.config_data['LOG_DIR']}/pr/wt_stats")
        os.system(f"mkdir -p {self.config_data['LOG_DIR']}/pr/iter_get")
        os.system(f"mkdir -p {self.config_data['LOG_DIR']}/pr/iter_map")
        pr_log = open(f"{self.log_file}_pr.log", "w")
        pr_log.write("PAGERANK\n\n")
        for ds in self.datasets:
            for graph_type in self.types:
                pr_log.write(f"\n\t{graph_type}_rd_{ds}\n")
                pr_log.write("\tREGULAR PR\n")
                for trials in range(1, 8):
                    # regular run, 8 runs
                    cmd = self.make_pr_cmd(
                        f"{self.config_data['RELEASE_PATH']}/benchmark/pagerank", ds, graph_type)
                    pr_log.write(f"{cmd} \n")
                    os.system(cmd)

                    cmd = self.make_pr_cmd(
                        f"{self.config_data['RELEASE_PATH']}/benchmark/pr_iter_map", ds, graph_type)
                    pr_log.write(f"{cmd} \n")
                    os.system(cmd)

                    cmd = self.make_pr_cmd(
                        f"{self.config_data['RELEASE_PATH']}/benchmark/pr_iter_getout", ds, graph_type)
                    pr_log.write(f"{cmd} \n")
                    os.system(cmd)

                # WT_STATS
                pr_log.write("\tWT_STATS\n")
                cmd = self.make_pr_cmd(
                    f"{self.config_data['PROFILE_PATH']}/benchmark/pagerank", ds, graph_type)
                pr_log.write(f"{cmd} \n")
                os.system(cmd)

                cmd = self.make_pr_cmd(
                    f"{self.config_data['PROFILE_PATH']}/benchmark/pr_iter_map", ds, graph_type)
                pr_log.write(f"{cmd} \n")
                os.system(cmd)

                cmd = self.make_pr_cmd(
                    f"{self.config_data['PROFILE_PATH']}/benchmark/pr_iter_getout", ds, graph_type)
                pr_log.write(f"{cmd} \n")
                os.system(cmd)

                # perf run
                pr_log.write("\tPERF\n")
                cmd = self.make_perf_params("pr", ds, graph_type) + " " + self.make_pr_cmd(
                    f"{self.config_data['STATS_PATH']}/benchmark/pagerank", ds, graph_type)
                pr_log.write(f"{cmd} \n")
                os.system(cmd)

                cmd = self.make_perf_params("pr", f"iter_map_{ds}", graph_type) + " " + self.make_pr_cmd(
                    f"{self.config_data['STATS_PATH']}/benchmark/pr_iter_map", ds, graph_type)
                pr_log.write(f"{cmd} \n")
                os.system(cmd)

                cmd = self.make_perf_params("pr", f"iter_getout_{ds}", graph_type) + " " + self.make_pr_cmd(
                    f"{self.config_data['STATS_PATH']}/benchmark/pr_iter_getout", ds, graph_type)
                pr_log.write(cmd)
                os.system(cmd)
                pr_log.write("\n" + ("~-" * 50) + "\n")
        pr_log.write("\n" + "=" * 100 + "\n")
        pr_log.close()

    def run_bfs(self):
        bfs_log = open(f"{self.log_file}_bfs.log", "w")
        bfs_log.write("\n\nBFS\n\n")
        for ds in self.datasets:
            for graph_type in self.types:
                cmd = self.make_bfs_cmd(
                    f"{self.config_data['RELEASE_PATH']}/benchmark/bfs", ds, graph_type)
                bfs_log.write(f"{cmd} \n")
                os.system(cmd)

                # WT_STATS
                bfs_log.write("\t\tWT_STATS\n")
                cmd = self.make_bfs_cmd(
                    f"{self.config_data['PROFILE_PATH']}/benchmark/bfs", ds, graph_type)
                bfs_log.write(f"{cmd} \n")
                os.system(cmd)

                # perf run
                bfs_log.write("\t\tPERF\n")
                cmd = self.make_perf_params("bfs", ds, graph_type) + " " + self.make_bfs_cmd(
                    f"{self.config_data['STATS_PATH']}/benchmark/bfs", ds, graph_type)
                bfs_log.write(f"{cmd} \n")
                os.system(cmd)
                bfs_log.write("\n" + ("~-" * 50) + "\n")
        bfs_log.write("\n" + "=" * 100 + "\n")
        bfs_log.close()

    def run_tc(self):
        tc_log = open(f"{self.log_file}_tc.log", "w")
        tc_log.write("\n\nTC\n\n")
        for ds in self.datasets:
            for graph_type in self.types:
                cmd = self.make_tc_cmd(
                    f"{self.config_data['RELEASE_PATH']}/benchmark/tc", ds, graph_type)
                tc_log.write(cmd)
                os.system(cmd)

                # WT_STATS
                tc_log.write("\t\tWT_STATS\n")
                cmd = self.make_tc_cmd(
                    f"{self.config_data['PROFILE_PATH']}/benchmark/tc", ds, graph_type)
                tc_log.write(cmd)
                os.system(cmd)

                # perf run
                tc_log.write("\t\tPERF\n")
                cmd = self.make_perf_params("tc", ds, graph_type) + " " + self.make_tc_cmd(
                    f"{self.config_data['STATS_PATH']}/benchmark/tc", ds, graph_type)
                tc_log.write(cmd)
                os.system(cmd)
                tc_log.write("\n" + ("~-" * 50) + "\n")

        tc_log.write("\n" + "=" * 100 + "\n")
        tc_log.close()

    def run_cc(self):
        cc_log = open(f"{self.log_file}_cc.log", "w")
        cc_log.write("CC\n\n")
        variants = ['cc', 'cc_parallel', 'cc_parallel_ec']
        for ds in self.datasets:
            for graph_type in self.types:
                for variant in variants:
                    cmd = self.make_cc_cmd(
                        f"{self.config_data['RELEASE_PATH']}/benchmark/{variant}", ds, graph_type, variant)
                    cc_log.write(cmd+"\n")
                    os.system(cmd)

                    # WT_STATS
                    cc_log.write("\tWT_STATS\n")
                    cmd = self.make_cc_cmd(
                        f"{self.config_data['PROFILE_PATH']}/benchmark/{variant}", ds, graph_type, variant)
                    cc_log.write(cmd+"\n")
                    os.system(cmd)

                    # perf run
                    cc_log.write("\tPERF\n")
                    cmd = self.make_perf_params("cc", ds, graph_type) + " " + self.make_cc_cmd(
                        f"{self.config_data['STATS_PATH']}/benchmark/{variant}", ds, graph_type, variant)
                    cc_log.write(cmd+"\n")
                    os.system(cmd)
                    cc_log.write("\n" + ("~-" * 50) + "\n")
        cc_log.write("\n" + "=" * 100 + "\n")
        cc_log.close()

    def run_sssp(self):
        sssp_log = open(f"{self.log_file}_sssp.log", "w")
        sssp_log.write("\n\nSSSP\n\n")
        for ds in self.datasets:
            for graph_type in self.types:
                cmd = self.make_sssp_cmd(
                    f"{self.config_data['RELEASE_PATH']}/benchmark/sssp", ds, graph_type)
                sssp_log.write(cmd)
                os.system(cmd)

                # WT_STATS
                sssp_log.write("\t\tWT_STATS\n")
                cmd = self.make_sssp_cmd(
                    f"{self.config_data['PROFILE_PATH']}/benchmark/sssp", ds, graph_type)
                sssp_log.write(cmd)
                os.system(cmd)

                # perf run
                sssp_log.write("\t\tPERF\n")
                cmd = self.make_perf_params("sssp", ds, graph_type) + " " + self.make_sssp_cmd(
                    f"{self.config_data['STATS_PATH']}/benchmark/sssp", ds, graph_type)
                sssp_log.write(cmd)
                os.system(cmd)
                sssp_log.write("\n" + ("~-" * 50) + "\n")
        sssp_log.write("\n" + "=" * 100 + "\n")
        sssp_log.close()


def main():

    # Set the group ID to 'graphs'
    group_id = grp.getgrnam("graphs").gr_gid
    os.setgid(group_id)
    assert (os.getgid() == group_id)

    parser = argparse.ArgumentParser(
        description="Run benchmarks"
    )
    parser.add_argument("--log_dir", type=str, help="log directory")
    parser.add_argument("--pr", action='store_true',
                        default=False, help="Run PageRank")
    parser.add_argument("--bfs", action='store_true',
                        default=False, help="Run BFS")
    parser.add_argument("--tc", action='store_true',
                        default=False, help="Run TC")
    parser.add_argument("--cc", action='store_true',
                        default=False, help="Run CC")
    parser.add_argument("--sssp", action='store_true',
                        default=False, help="Run SSSP")

    args = parser.parse_args()
    config_data = ConfigReader("config.json").read_config()

    if not (args.pr or args.bfs or args.tc or args.cc or args.sssp):
        print("No benchmark selected. Exiting")
        exit(0)

    if args.log_dir is None:
        print("Using CWD as log directory")
        git_cmd = f"git rev-parse --short HEAD"
        git_id = subprocess.check_output(git_cmd, shell=True)
        log_dir = os.path.join(
            config_data['LOG_DIR'], git_id.decode('ascii').strip())
        os.system(f"mkdir -p {log_dir}")
        config_data['LOG_DIR'] = log_dir
    else:
        config_data.update(vars(args))

    benchmark_runner = BenchmarkRunner(
        config_data, config_data['LOG_DIR'] + "/benchmark")

    if args.pr:
        os.system(f"mkdir -p {config_data['LOG_DIR']}/pr")
        benchmark_runner.run_pr()

    if args.tc:
        os.system(f"mkdir -p {config_data['LOG_DIR']}/tc")
        benchmark_runner.run_tc()

    if args.bfs:
        os.system(f"mkdir -p {config_data['LOG_DIR']}/bfs")
        benchmark_runner.run_bfs()
    if args.cc:
        os.system(f"mkdir -p {config_data['LOG_DIR']}/cc")
        benchmark_runner.run_cc()
    if args.sssp:
        os.system(f"mkdir -p {config_data['LOG_DIR']}/sssp")
        benchmark_runner.run_sssp()


if __name__ == "__main__":
    main()
