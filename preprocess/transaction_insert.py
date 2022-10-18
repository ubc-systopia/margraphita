import argparse
import os
import time
from subprocess import check_output
from datetime import datetime
from subprocess import check_output
from paths import ConfigReader

'''
We insert the graph into the database using the GraphAPI and compare the performance of single threaded inserts with transactions turned on and off.
'''


class TransactionInsert:
    def __init__(self, config_data: dict[str, any]):
        self.config_data = config_data
        self.log_file = os.path.join(
            config_data['log_dir'], "transaction_compare.log")
        self.log = open(self.log_file, "w")
        self.date = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        self.log_entry = "Scale: " + \
                         str(self.config_data['scale']) + "\nPath:" + self.config_data[
                             'graph_path'] + "\nDate: " + self.date + "\n"
        self.log.write(self.log_entry)

    def run_tx(self, graph_type: str):
        tx_cmd = (f"{self.config_data['RELEASE_PATH']}/preprocess/mt_graphapi_insert"
                  f" -d {graph_type}_rd_{self.config_data['dataset_name']}"
                  f" -e {self.config_data['num_edges']}"
                  f" -n {self.config_data['num_nodes']}"
                  f" -f {self.config_data['graph_path']}"
                  f" -t {graph_type} -p {self.config_data['output_dir']}"
                  f" -r -l {self.config_data['log_dir']}")
        print(tx_cmd)

    def run_no_tx(self, graph_type: str):
        notx_cmd = (f"{self.config_data['GRAPH_PROJECT_DIR']}/no_tx/build/release/preprocess/mt_graphapi_insert_notx"
                    f" -d {graph_type}_rd_{self.config_data['dataset_name']}"
                    f" -e {self.config_data['num_edges']}"
                    f" -n {self.config_data['num_nodes']}"
                    f" -f {self.config_data['graph_path']}"
                    f" -t {graph_type} -p {self.config_data['output_dir']}"
                    f" -r -l {self.config_data['log_dir']}")
        print(notx_cmd)


def main():
    parser = argparse.ArgumentParser(description=(
        "Preprocess the graph and" "insert into the database We assume the graph has already been"
        "preprocessed. We measure times for read optimized graphs only."))
    required_args = parser.add_argument_group("Required arguments")
    parser.add_argument("--log_dir", type=str, help="log directory")
    parser.add_argument("--dataset_name", type=str,
                        help="dataset name", required=True)
    parser.add_argument("--graph_dir", type=str,
                        help="graph directory", required=True)
    parser.add_argument("--graph_path", type=str,
                        help="graph path", required=True)
    parser.add_argument("--num_nodes", type=int,
                        help="number of nodes")
    parser.add_argument("--num_edges", type=int,
                        help="number of edges")
    parser.add_argument("--graph_type", type=str,
                        help="graph type", required=True)
    parser.add_argument("--output_dir", type=str,
                        help="output directory", required=True)
    parser.add_argument("--index", action='store_true',
                        default=False, help="create index")
    parser.add_argument("--scale", type=int, help="scale", required=True)

    args = parser.parse_args()
    config_data = ConfigReader("config.json").read_config()
    config_data.update(vars(args))

    if args.log_dir is None:
        print("Using CWD as log directory")
        config_data['log_dir'] = os.getcwd()
    if args.num_nodes is None:
        print("Getting number of nodes from graph")
        config_data['num_nodes'] = int(
            check_output(["wc", "-l", f"{args.graph_path}_nodes"]).split()[0])
    if args.num_edges is None:
        print("Getting number of edges from graph")
        config_data['num_edges'] = int(
            check_output(["wc", "-l", f"{args.graph_path}"]).split()[0])

    transaction_insert = TransactionInsert(config_data)
    for graph_type in ["std", "ekey", "adj"]:
        transaction_insert.run_tx(graph_type)
        transaction_insert.run_no_tx(graph_type)


if __name__ == "__main__":
    main()
