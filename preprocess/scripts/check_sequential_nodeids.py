import sys
import re

'''
This script is used to parse the output from wt dump and verify that the node IDs are sequential
'''


def main():
    if len(sys.argv) != 3:
        print("Usage: python3 check_sequential_nodeids.py <type> <path_to_wt_dump_of_nodes_table>")
        exit(1)
    wt_dump_path = sys.argv[2]
    wt_type = sys.argv[1]
    
    if wt_type == "std" or wt_type == "adj":
        expected_node_id = 0
        with open(wt_dump_path, 'r') as file_handle:
            for line in file_handle:
                if not line.startswith("\"id\" : "):
                    continue
                else:
                    node_id = line.split('\"id\" : ')[1].split(',')[0]
                    assert int(node_id) == expected_node_id, "Node IDs are not sequential"
                    expected_node_id += 1
        print("Node IDs are sequential")
    
    elif wt_type == "ekey":
        expected_node_id = 0
        # node_rexp = re.compile(r'"src"\s:\s(\d+),\n"dst"\s+:\s-1')
        with open(wt_dump_path, 'r') as file_handle:
            for line in file_handle:
                if line.startswith("\"src\" : "):
                    node_id = line.split('\"src\" : ')[1].split(',')[0]
                    next = file_handle.readline()
                    if next.startswith("\"dst\" : -1"):
                        assert int(node_id) == expected_node_id, "Node IDs are not sequential"
                        expected_node_id += 1
                else:
                    continue
        print("Node IDs are sequential")



if __name__ == "__main__":
    main()
