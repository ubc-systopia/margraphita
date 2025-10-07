#!/usr/bin/env python3
"""
Verify that the adjacency list and weights files are correctly generated.
This script checks that for a given input edge file, the output adjacency list
and weights files contain the correct data.
"""

import sys
from collections import defaultdict
from typing import Dict, List, Tuple


def parse_input_edges(filename: str, weighted: bool = False) -> Tuple[Dict[int, Tuple[List[int], List[float]]], Dict[int, int]]:
    """
    Parse input edge file and build expected adjacency lists and weights.
    Returns a tuple of:
    - dictionary mapping node_id -> (edgelist, weights)
    - dictionary mapping node_id -> first_line_number where node appears
    """
    adj_dict = defaultdict(lambda: ([], []))
    node_line_map = {}

    with open(filename, 'r') as f:
        for line_num, line in enumerate(f, 1):
            parts = line.strip().split()
            if len(parts) < 2:
                continue

            src = int(parts[0])
            dst = int(parts[1])
            weight = float(parts[2]) if weighted and len(parts) > 2 else 0.0

            # Track first line number for this node
            if src not in node_line_map:
                node_line_map[src] = line_num

            # Get current lists
            edgelist, weights = adj_dict[src]
            edgelist.append(dst)
            if weighted:
                weights.append(weight)
            adj_dict[src] = (edgelist, weights)

    return dict(adj_dict), node_line_map


def parse_adjlist_file(filename: str) -> Tuple[Dict[int, Tuple[int, List[int]]], Dict[int, int]]:
    """
    Parse adjacency list output file.
    Returns a tuple of:
    - dictionary mapping node_id -> (degree, edgelist)
    - dictionary mapping node_id -> line_number in output file
    """
    adj_dict = {}
    line_map = {}

    with open(filename, 'r') as f:
        for line_num, line in enumerate(f, 1):
            try:
                parts = line.strip().split()
                if len(parts) < 2:
                    print(f"Warning: Line {line_num} has fewer than 2 parts: {line.strip()[:100]}")
                    continue

                node_id = int(parts[0])
                degree = int(parts[1])

                # Parse edge list (comma-separated)
                edgelist = []
                if len(parts) > 2:
                    edges_str = ' '.join(parts[2:])
                    edgelist = [int(x.strip()) for x in edges_str.split(',') if x.strip()]

                adj_dict[node_id] = (degree, edgelist)
                line_map[node_id] = line_num
            except ValueError as e:
                print(f"Error parsing line {line_num} in {filename}: {e}")
                print(f"Line content (first 200 chars): {line.strip()[:200]}")
                raise

    return adj_dict, line_map


def parse_weights_file(filename: str) -> Tuple[Dict[int, List[float]], Dict[int, int]]:
    """
    Parse weights output file.
    Returns a tuple of:
    - dictionary mapping node_id -> weights
    - dictionary mapping node_id -> line_number in weights file
    """
    weights_dict = {}
    line_map = {}

    try:
        with open(filename, 'r') as f:
            for line_num, line in enumerate(f, 1):
                try:
                    parts = line.strip().split()
                    if len(parts) < 1:
                        continue

                    node_id = int(parts[0])

                    # Parse weights (comma-separated)
                    weights = []
                    if len(parts) > 1:
                        weights_str = ' '.join(parts[1:])
                        weights = [float(x.strip()) for x in weights_str.split(',') if x.strip()]

                    weights_dict[node_id] = weights
                    line_map[node_id] = line_num
                except ValueError as e:
                    print(f"Error parsing line {line_num} in {filename}: {e}")
                    print(f"Line content (first 200 chars): {line.strip()[:200]}")
                    raise
    except FileNotFoundError:
        print(f"Warning: Weights file {filename} not found")
        return {}, {}

    return weights_dict, line_map


def get_next_chunk_files(adjlist_file: str, weights_file: str = None) -> Tuple[str, str]:
    """
    Get the next chunk file names.
    E.g., out_aa -> out_ab, weights_aa -> weights_ab
    """
    import re

    # Extract the suffix (e.g., _aa)
    match = re.search(r'_([a-z]{2})$', adjlist_file)
    if not match:
        return None, None

    suffix = match.group(1)
    prefix = adjlist_file[:match.start()]

    # Increment the suffix
    if len(suffix) == 2:
        first_char = ord(suffix[0])
        second_char = ord(suffix[1])

        second_char += 1
        if second_char > ord('z'):
            second_char = ord('a')
            first_char += 1

        if first_char > ord('z'):
            return None, None  # No next chunk

        next_suffix = chr(first_char) + chr(second_char)
        next_adjlist = prefix + "_" + next_suffix

        next_weights = None
        if weights_file:
            match_w = re.search(r'_([a-z]{2})$', weights_file)
            if match_w:
                weights_prefix = weights_file[:match_w.start()]
                next_weights = weights_prefix + "_" + next_suffix

        return next_adjlist, next_weights

    return None, None


def verify_output(input_file: str, adjlist_file: str, weights_file: str = None, weighted: bool = False):
    """
    Verify that the output files match the expected values from the input file.
    """
    print(f"Parsing input file: {input_file}")
    expected, input_line_map = parse_input_edges(input_file, weighted)

    print(f"Parsing adjacency list file: {adjlist_file}")
    actual_adj, adj_line_map = parse_adjlist_file(adjlist_file)

    actual_weights = {}
    weights_line_map = {}
    if weighted and weights_file:
        print(f"Parsing weights file: {weights_file}")
        actual_weights, weights_line_map = parse_weights_file(weights_file)

    # Try to load next chunk files for conflict resolution
    next_adjlist_file, next_weights_file = get_next_chunk_files(adjlist_file, weights_file)
    next_adj = {}
    next_adj_line_map = {}
    next_weights = {}
    next_weights_line_map = {}

    if next_adjlist_file:
        try:
            print(f"Checking next chunk file for conflict resolution: {next_adjlist_file}")
            next_adj, next_adj_line_map = parse_adjlist_file(next_adjlist_file)
            if weighted and next_weights_file:
                print(f"Checking next weights file for conflict resolution: {next_weights_file}")
                next_weights, next_weights_line_map = parse_weights_file(next_weights_file)
        except FileNotFoundError:
            print(f"Note: Next chunk file {next_adjlist_file} not found (this is OK if it's the last chunk)")
        except Exception as e:
            print(f"Warning: Could not read next chunk file: {e}")

    # Track nodes that are in next chunk for ordering verification
    nodes_in_next_chunk = []

    print("\n" + "="*80)
    print("VERIFICATION RESULTS")
    print("="*80 + "\n")

    errors = 0
    warnings = 0

    # Get all unique node IDs from input in order to find last node
    node_ids = list(expected.keys())
    last_node_id = node_ids[-1] if node_ids else None

    # Check all expected nodes are present
    for node_id in expected.keys():
        expected_edgelist, expected_weights = expected[node_id]
        input_line = input_line_map.get(node_id, "?")

        if node_id not in actual_adj:
            # Check if it's in the next chunk (conflict resolution)
            if node_id in next_adj:
                print(f"ℹ️  INFO: Node {node_id} (input line {input_line}) not in current chunk but found in next chunk (line {next_adj_line_map.get(node_id, '?')}) due to conflict resolution")
                actual_degree, actual_edgelist = next_adj[node_id]
                adj_line = next_adj_line_map.get(node_id, "?")

                # Track this node for ordering verification
                nodes_in_next_chunk.append((node_id, input_line, adj_line))

                if weighted and node_id in next_weights:
                    actual_node_weights = next_weights[node_id]
                    weights_line = next_weights_line_map.get(node_id, "?")
                else:
                    actual_node_weights = []
                    weights_line = "?"
            else:
                print(f"❌ ERROR: Node {node_id} (input line {input_line}) missing from adjacency list file")
                errors += 1
                continue
        else:
            actual_degree, actual_edgelist = actual_adj[node_id]
            adj_line = adj_line_map.get(node_id, "?")

            if weighted:
                actual_node_weights = actual_weights.get(node_id, [])
                weights_line = weights_line_map.get(node_id, "?")

        # Check degree matches edgelist size
        if actual_degree != len(actual_edgelist):
            print(f"❌ ERROR: Node {node_id} (input line {input_line}, adjlist line {adj_line}): degree {actual_degree} != edgelist size {len(actual_edgelist)}")
            errors += 1

        # Check edgelist matches expected
        if len(expected_edgelist) != len(actual_edgelist):
            print(f"❌ ERROR: Node {node_id} (input line {input_line}, adjlist line {adj_line}): expected {len(expected_edgelist)} edges, got {len(actual_edgelist)}")
            print(f"   Expected: {expected_edgelist}")
            print(f"   Actual:   {actual_edgelist}")
            errors += 1
        elif expected_edgelist != actual_edgelist:
            print(f"⚠️  WARNING: Node {node_id} (input line {input_line}, adjlist line {adj_line}): edgelist content differs")
            print(f"   Expected: {expected_edgelist}")
            print(f"   Actual:   {actual_edgelist}")
            warnings += 1

        # Check weights if weighted graph
        if weighted:
            if len(expected_weights) != len(actual_node_weights):
                print(f"❌ ERROR: Node {node_id} (input line {input_line}, weights line {weights_line}): expected {len(expected_weights)} weights, got {len(actual_node_weights)}")
                print(f"   Expected: {expected_weights}")
                print(f"   Actual:   {actual_node_weights}")
                errors += 1
            elif expected_weights != actual_node_weights:
                # Check if weights are approximately equal (floating point)
                if all(abs(e - a) < 1e-6 for e, a in zip(expected_weights, actual_node_weights)):
                    continue
                print(f"❌ ERROR: Node {node_id} (input line {input_line}, weights line {weights_line}): weights differ")
                print(f"   Expected: {expected_weights}")
                print(f"   Actual:   {actual_node_weights}")
                errors += 1

            # Check weights size matches edgelist size
            if len(actual_node_weights) != len(actual_edgelist):
                print(f"❌ ERROR: Node {node_id} (input line {input_line}, adjlist line {adj_line}, weights line {weights_line}): weights size {len(actual_node_weights)} != edgelist size {len(actual_edgelist)}")
                errors += 1

    # Check for extra nodes in output
    for node_id in actual_adj.keys():
        if node_id not in expected:
            adj_line = adj_line_map.get(node_id, "?")
            print(f"⚠️  WARNING: Node {node_id} (adjlist line {adj_line}) in output but not in input")
            warnings += 1

    # Verify ordering of nodes that moved to next chunk
    if nodes_in_next_chunk:
        print("\n" + "="*80)
        print("CHUNK BOUNDARY ORDERING VERIFICATION")
        print("="*80 + "\n")

        # Sort by input line number to get expected order
        nodes_in_next_chunk.sort(key=lambda x: x[1])

        # Check if they appear in the same order in the next chunk (by adj_line)
        print(f"Found {len(nodes_in_next_chunk)} node(s) that moved to next chunk:")
        prev_adj_line = 0
        ordering_errors = 0

        for i, (node_id, input_line, adj_line) in enumerate(nodes_in_next_chunk):
            print(f"  Node {node_id}: input line {input_line} -> next chunk line {adj_line}")

            if isinstance(adj_line, int) and adj_line <= prev_adj_line:
                print(f"    ❌ ERROR: Node {node_id} appears out of order in next chunk (line {adj_line} <= {prev_adj_line})")
                ordering_errors += 1

            if isinstance(adj_line, int):
                prev_adj_line = adj_line

        # Verify they appear at the beginning of the next chunk
        if next_adj:
            next_chunk_node_ids = list(next_adj.keys())
            expected_at_start = [n[0] for n in nodes_in_next_chunk]

            # Check if the first N nodes in next chunk match the moved nodes
            actual_at_start = next_chunk_node_ids[:len(expected_at_start)]

            if expected_at_start != actual_at_start:
                print(f"\n  ❌ ERROR: Nodes do not appear at the start of next chunk in expected order")
                print(f"    Expected first {len(expected_at_start)} nodes: {expected_at_start}")
                print(f"    Actual first {len(actual_at_start)} nodes: {actual_at_start}")
                ordering_errors += 1
            else:
                print(f"\n  ✅ Nodes appear at the start of next chunk in correct order")

        if ordering_errors > 0:
            errors += ordering_errors

    print("\n" + "="*80)
    print(f"Total nodes in input: {len(expected)}")
    print(f"Total nodes in output: {len(actual_adj)}")
    print(f"Errors: {errors}")
    print(f"Warnings: {warnings}")

    if errors == 0 and warnings == 0:
        print("\n✅ ALL CHECKS PASSED!")
        return 0
    elif errors == 0:
        print(f"\n⚠️  ALL CHECKS PASSED WITH {warnings} WARNINGS")
        return 0
    else:
        print(f"\n❌ VERIFICATION FAILED WITH {errors} ERRORS")
        return 1


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python verify_output.py <input_edge_file> <adjlist_output_file> [weights_output_file] [--weighted]")
        print()
        print("Examples:")
        print("  # Unweighted graph")
        print("  python verify_output.py dataset_aa in_aa")
        print()
        print("  # Weighted graph")
        print("  python verify_output.py dataset_aa in_aa weights_aa --weighted")
        sys.exit(1)

    input_file = sys.argv[1]
    adjlist_file = sys.argv[2]
    weights_file = None
    weighted = False

    if len(sys.argv) > 3:
        if "--weighted" in sys.argv:
            weighted = True
            # Find weights file (the arg before --weighted or after adjlist_file)
            for i, arg in enumerate(sys.argv[3:], 3):
                if arg != "--weighted":
                    weights_file = arg
                    break
        else:
            weights_file = sys.argv[3]
            if len(sys.argv) > 4 and sys.argv[4] == "--weighted":
                weighted = True

    sys.exit(verify_output(input_file, adjlist_file, weights_file, weighted))
