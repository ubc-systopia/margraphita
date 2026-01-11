import argparse
import os
import subprocess
import time
from datetime import datetime
from subprocess import check_output
from paths import ConfigReader
from dataset_properties import PropertiesReader
import grp


class BenchmarkRunner:
    def __init__(self, config_data: dict[str, any]):
        # Use datasets from config if provided, otherwise use default list
        self.datasets = config_data.get('datasets', ["soc-LiveJournal", "dota_league","graph500_26", "graph500_28", "graph500_30", "uniform_26", "twitter_mpi","uk-2007", "com-friendster"])

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

    def get_properties_reader(self, ds: str):
        """
        Create a PropertiesReader for the given dataset.
        Assumes properties files are at /datasets/<dataset>/<dataset>.properties
        """
        dataset_path = f"{self.config_data['ds_dir']}/{ds}"
        print(f"Dataset path for properties: {dataset_path}")
        props_reader = PropertiesReader(ds, dataset_path, system_name='flexograph')
        return props_reader

    def make_pr_cmd(self, binary_name: str, ds: str, graph_type: str):
        props_reader = self.get_properties_reader(ds)
        direction_suffix = "rd" if props_reader.is_directed() else "r"
        cmd = f"{binary_name} -m {graph_type}_{direction_suffix}_{ds} -g {graph_type} -p {self.config_data['DB_DIR']}/{ds} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else ""
        cmd+= f" >> {self.config_data['LOG_DIR']}/{ds}_pr_{graph_type}.log"
        return cmd

    def make_bfs_cmd(self, binary_name: str, ds: str, graph_type: str, vert: int):
        props_reader = self.get_properties_reader(ds)
        direction_suffix = "rd" if props_reader.is_directed() else "r"
        cmd = f"{binary_name} -m {graph_type}_{direction_suffix}_{ds} -p {self.config_data['DB_DIR']}/{ds} -g {graph_type} -v {vert} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else ""
        cmd+= f">> {self.config_data['LOG_DIR']}/{ds}_bfs_{graph_type}.log"
        return cmd

    def make_bc_cmd(self, binary_name: str, ds: str, graph_type: str, vert: int):
        props_reader = self.get_properties_reader(ds)
        direction_suffix = "rd" if props_reader.is_directed() else "r"
        cmd = f"{binary_name} -m {graph_type}_{direction_suffix}_{ds} -p {self.config_data['DB_DIR']}/{ds} -g {graph_type} -v {vert} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else ""
        cmd+= f" >> {self.config_data['LOG_DIR']}/{ds}_bc_{graph_type}.log"
        return cmd

    def make_tc_cmd(self, binary_name: str, ds: str, graph_type: str):
        props_reader = self.get_properties_reader(ds)
        direction_suffix = "rd" if props_reader.is_directed() else "r"
        cmd = f"{binary_name} -m {graph_type}_{direction_suffix}_{ds} -p {self.config_data['DB_DIR']}/{ds} -g {graph_type} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else ""
        cmd+= f" >> {self.config_data['LOG_DIR']}/{ds}_tc_{graph_type}.log"
        return cmd

    def make_cc_cmd(self, binary_name: str, ds: str, graph_type: str, variant: str):
        props_reader = self.get_properties_reader(ds)
        direction_suffix = "rd" if props_reader.is_directed() else "r"
        cmd = f"{binary_name} -m {graph_type}_{direction_suffix}_{ds} -p {self.config_data['DB_DIR']}/{ds} -g {graph_type} "
        cmd+= f"-z {self.config_data['config_string']} " if 'config_string' in self.config_data else ""
        cmd+= f">> {self.config_data['LOG_DIR']}/{ds}_cc_{graph_type}.log"
        return cmd

    def make_sssp_cmd(self, binary_name: str, ds: str, graph_type: str, vert: int):
        props_reader = self.get_properties_reader(ds)
        direction_suffix = "rd" if props_reader.is_directed() else "r"
        cmd = f"{binary_name} -m {graph_type}_{direction_suffix}_{ds} -p {self.config_data['DB_DIR']}/{ds} -g {graph_type} -v {vert} "
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
            # Use PropertiesReader to get the source vertex
            props_reader = self.get_properties_reader(ds)
            props = props_reader.read()

            if props is None or props['bfs_source'] is None:
                print(f"Warning: Could not find BFS source vertex for {ds}, Skipping")
                continue
            else:
                # Use the source vertex from properties file
                random_verts = [int(props['bfs_source'])]

            for graph_type in self.types:
                for vert in random_verts:
                    cmd = self.make_bfs_cmd(f"{self.config_data['RELEASE_PATH']}/benchmark/bfs_parallel", ds, graph_type, vert)
                    self.log(cmd)
                    if(self.config_data['dry_run']):
                        continue
                    os.system(cmd)
    
    def bc(self):
        for ds in self.datasets:
            # Use PropertiesReader to get the source vertex
            props_reader = self.get_properties_reader(ds)
            props = props_reader.read()

            if props is None or props['bfs_source'] is None:
                print(f"Warning: Could not find BC source vertex for {ds}, Skipping")
                continue
            else:
                # Use the source vertex from properties file (BC can use same as BFS)
                source = props['bfs_source']
                random_verts = [int(source)]

            for graph_type in self.types:
                for vert in random_verts:
                    cmd = self.make_bc_cmd(
                        f"{self.config_data['RELEASE_PATH']}/benchmark/bc_parallel", ds, graph_type, vert)
                    self.log(cmd)
                    if(self.config_data['dry_run']):
                        continue
                    os.system(cmd)

    def tc(self):
        for ds in self.datasets:
            for graph_type in self.types:
                cmd = self.make_tc_cmd(
                    f"{self.config_data['RELEASE_PATH']}/benchmark/tc_gap_optimized", ds, graph_type)
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
            # Use PropertiesReader to get the source vertex
            props_reader = self.get_properties_reader(ds)
            props = props_reader.read()

            if props is None or props['sssp_source'] is None:
                print(f"Warning: Could not find SSSP source vertex for {ds}, falling back to .bfsver file")
                # Fallback to old method
                random_verts = None
                with open(f"{self.config_data['DB_DIR']}/{ds}/{ds}.bfsver", "r") as random_file:
                    random_verts = random_file.readlines()
                random_verts = [int(v.strip()) for v in random_verts]
            else:
                # Use the source vertex from properties file (SSSP can use same as BFS)
                source = props['sssp_source'] if props['sssp_source'] else props['bfs_source']
                random_verts = [int(source)]

            for graph_type in self.types:
                for vert in random_verts:
                    cmd = self.make_sssp_cmd(
                        f"{self.config_data['RELEASE_PATH']}/benchmark/sssp_parallel", ds, graph_type, vert)
                    self.log(cmd)
                    if(self.config_data['dry_run']):
                        continue
                    os.system(cmd)


def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description='Run graph benchmarks')
    parser.add_argument('--config', type=str, required=True, help='Path to config file')
    parser.add_argument('--ds_dir', type=str, help='Directory containing datasets')
    parser.add_argument('--datasets', type=str, nargs='+', help='List of datasets to process')
    parser.add_argument('--dry_run', action='store_true', help='Print commands without executing')
    args = parser.parse_args()

    # Read config
    config_reader = ConfigReader(args.config)
    config_data = config_reader.read_config()
    config_data['dry_run'] = args.dry_run
    config_data['ds_dir'] = args.ds_dir if args.ds_dir else config_data.get('ds_dir', '/datasets')
    if args.datasets:
        config_data['datasets'] = args.datasets

    runner = BenchmarkRunner(config_data)

    # Iterate through datasets and run all supported algorithms
    for ds in runner.datasets:
        print(f"\n{'='*80}")
        print(f"Processing dataset: {ds}")
        print(f"{'='*80}")

        # Get properties for this dataset
        props_reader = runner.get_properties_reader(ds)
        props = props_reader.read()

        if props is None:
            print(f"Could not read properties for {ds}, skipping")
            continue

        print(f"  Supported algorithms: {props['algorithms']}")
        print(f"  Directed: {props_reader.is_directed()}")
        print(f"  Weighted: {props_reader.is_weighted()}")

        # Get mapped algorithm names for flexograph
        benchmarks_no_source = props_reader.get_benchmarks_no_source()
        benchmarks_with_source = props_reader.get_benchmarks_requiring_source()

        print(f"  Benchmarks without source: {benchmarks_no_source}")
        print(f"  Benchmarks with source: {benchmarks_with_source}")

        # Run benchmarks that don't need source vertex
        for graph_type in runner.types:
            # PageRank
            if 'pr_vc' in benchmarks_no_source:
                cmd = runner.make_pr_cmd(
                    f"{config_data['RELEASE_PATH']}/benchmark/pr_vc", ds, graph_type)
                print(f"\nRunning PageRank: {cmd}")
                runner.log(cmd)
                if not config_data['dry_run']:
                    os.system(cmd)

            # Connected Components
            if 'cc_parallel' in benchmarks_no_source:
                cmd = runner.make_cc_cmd(
                    f"{config_data['RELEASE_PATH']}/benchmark/cc_parallel", ds, graph_type, "cc_parallel")
                print(f"\nRunning Connected Components: {cmd}")
                runner.log(cmd)
                if not config_data['dry_run']:
                    os.system(cmd)

            # Triangle Counting
            if 'tc_gap' in benchmarks_no_source:
                cmd = runner.make_tc_cmd(
                    f"{config_data['RELEASE_PATH']}/benchmark/tc", ds, graph_type)
                print(f"\nRunning Triangle Counting: {cmd}")
                runner.log(cmd)
                if not config_data['dry_run']:
                    os.system(cmd)

        # Run benchmarks that need source vertex
        if benchmarks_with_source:
            # Get source vertex
            source_vertex = props_reader.get_source_vertex()

            if source_vertex is None:
                print(f"  Warning: No source vertex found for {ds}, skipping source-based algorithms")
            else:
                print(f"  Using source vertex: {source_vertex}")

                for graph_type in runner.types:
                    # BFS
                    if 'bfs_parallel' in benchmarks_with_source and source_vertex is not None:
                        cmd = runner.make_bfs_cmd(
                            f"{config_data['RELEASE_PATH']}/benchmark/bfs_parallel", ds, graph_type, int(source_vertex))
                        print(f"\nRunning BFS: {cmd}")
                        runner.log(cmd)
                        if not config_data['dry_run']:
                            os.system(cmd)

                    # SSSP
                    if 'sssp_parallel' in benchmarks_with_source and source_vertex is not None:
                        # For SSSP, check if there's a specific SSSP source, otherwise use BFS source
                        sssp_source = props['sssp_source'] if props['sssp_source'] else source_vertex
                        cmd = runner.make_sssp_cmd(
                            f"{config_data['RELEASE_PATH']}/benchmark/sssp_parallel", ds, graph_type, int(sssp_source))
                        print(f"\nRunning SSSP: {cmd}")
                        runner.log(cmd)
                        if not config_data['dry_run']:
                            os.system(cmd)

                    # Betweenness Centrality
                    if 'bc_parallel' in benchmarks_with_source and source_vertex is not None:
                        cmd = runner.make_bc_cmd(
                            f"{config_data['RELEASE_PATH']}/benchmark/bc_parallel", ds, graph_type, int(source_vertex))
                        print(f"\nRunning BC: {cmd}")
                        runner.log(cmd)
                        if not config_data['dry_run']:
                            os.system(cmd)

    runner.log_handle.write(f"Benchmark run completed at {datetime.now()}\n")
    runner.log_handle.close()
    print(f"\n{'='*80}")
    print(f"All benchmarks completed!")
    print(f"{'='*80}")


if __name__ == "__main__":
    main()
