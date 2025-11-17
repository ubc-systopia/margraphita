#!/usr/bin/env python3
"""
Script to test different OMP schedule configurations and partition scales
for the triangle counting benchmark.
"""

import subprocess
import sys
import os
import re
import time
from pathlib import Path
from typing import List, Tuple
import json

# Configuration arrays - modify these to test different values
OMP_SCHEDULES = [
    "schedule(dynamic, 1)",
    "schedule(dynamic, 2)",
    "schedule(dynamic, 4)",
    "schedule(dynamic, 8)",
    "schedule(dynamic, 16)",
    "schedule(dynamic, 32)",
    "schedule(dynamic, 64)",
    "schedule(guided)",
]

PARTITION_SCALES = [100, 125, 150, 175, 200, 300]

# Paths
BENCHMARK_DIR = "/home/puneet/scratch/margraphita/build/release"
CPP_FILE = "/home/puneet/scratch/margraphita/benchmark/tc_gapbs_optimized.cpp"
RESULTS_FILE = BENCHMARK_DIR / "optimization_results.json"

# Build and run commands
BUILD_CMD = ["make", "-j"]
RUN_CMD = [
    "./tc_gap_optimized",
    "-m", "adj_r_graph500_26",
    "-p", "/drives/graphs_backup/db/graph500_26",
    "-g", "adj",
    "-z", "cache_size=20G",
    "-#", "1"
]


def read_cpp_file() -> str:
    """Read the current content of the cpp file."""
    with open(CPP_FILE, 'r') as f:
        return f.read()


def backup_cpp_file(content: str):
    """Create a backup of the original file."""
    backup_file = CPP_FILE.with_suffix('.cpp.backup')
    with open(backup_file, 'w') as f:
        f.write(content)
    print(f"Backup created: {backup_file}")


def restore_cpp_file():
    """Restore the original file from backup."""
    backup_file = CPP_FILE.with_suffix('.cpp.backup')
    if backup_file.exists():
        with open(backup_file, 'r') as f:
            content = f.read()
        with open(CPP_FILE, 'w') as f:
            f.write(content)
        print(f"Restored from backup: {backup_file}")


def modify_cpp_file(schedule: str, partition_scale: int) -> bool:
    """
    Modify the cpp file with new schedule and partition scale values.
    Returns True if modifications were successful.
    """
    content = read_cpp_file()
    original_content = content

    # Modify OMP schedule (around line 33-34)
    # Pattern: #pragma omp parallel for ... schedule(...) ...
    schedule_pattern = r'(#pragma omp parallel for[^\n]*?)schedule\([^)]+\)'
    new_schedule_line = r'\1' + schedule
    content = re.sub(schedule_pattern, new_schedule_line, content)

    # Modify partition scale (line 194)
    partition_pattern = r'graphEngine\.set_partition_scale\(\s*\d+\s*\)'
    new_partition_line = f'graphEngine.set_partition_scale({partition_scale})'
    content = re.sub(partition_pattern, new_partition_line, content)

    # Write the modified content
    with open(CPP_FILE, 'w') as f:
        f.write(content)

    # Verify changes were made
    if content == original_content:
        print("WARNING: No changes were made to the file!")
        return False

    print(f"Modified: schedule={schedule}, partition_scale={partition_scale}")
    return True


def build_benchmark() -> bool:
    """
    Build the benchmark using make.
    Returns True if build was successful.
    """
    print("Building benchmark...")
    try:
        result = subprocess.run(
            BUILD_CMD,
            cwd=BENCHMARK_DIR,
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout
        )

        if result.returncode != 0:
            print(f"Build failed with return code {result.returncode}")
            print("STDERR:", result.stderr)
            return False

        print("Build successful")
        return True
    except subprocess.TimeoutExpired:
        print("Build timed out!")
        return False
    except Exception as e:
        print(f"Build error: {e}")
        return False


def run_benchmark() -> Tuple[bool, dict]:
    """
    Run the benchmark and parse the output.
    Returns (success, results_dict)
    """
    print("Running benchmark...")
    try:
        result = subprocess.run(
            RUN_CMD,
            cwd=BENCHMARK_DIR,
            capture_output=True,
            text=True,
            timeout=3600  # 1 hour timeout
        )

        output = result.stdout + result.stderr
        print("Benchmark output:")
        print(output)

        # Parse results from output
        results = {
            'success': result.returncode == 0,
            'return_code': result.returncode,
            'output': output
        }

        # Extract timing information
        time_match = re.search(r'Trust Triangle_Counting_ITER completed in : ([\d.]+)', output)
        if time_match:
            results['execution_time'] = float(time_match.group(1))

        # Extract triangle count
        count_match = re.search(r'Trust Triangles count = (\d+)', output)
        if count_match:
            results['triangle_count'] = int(count_match.group(1))

        # Extract average time
        avg_match = re.search(r'Average time Trust: ([\d.]+)', output)
        if avg_match:
            results['average_time'] = float(avg_match.group(1))

        # Extract number of partitions
        partitions_match = re.search(r'creating (\d+) partitions', output)
        if partitions_match:
            results['num_partitions'] = int(partitions_match.group(1))

        return (result.returncode == 0, results)

    except subprocess.TimeoutExpired:
        print("Benchmark timed out!")
        return (False, {'error': 'timeout'})
    except Exception as e:
        print(f"Run error: {e}")
        return (False, {'error': str(e)})


def save_results(all_results: List[dict]):
    """Save results to JSON file."""
    with open(RESULTS_FILE, 'w') as f:
        json.dump(all_results, indent=2, fp=f)
    print(f"\nResults saved to: {RESULTS_FILE}")


def print_summary(all_results: List[dict]):
    """Print a summary of all results."""
    print("\n" + "="*80)
    print("OPTIMIZATION RESULTS SUMMARY")
    print("="*80)

    successful_results = [r for r in all_results if r.get('run_success', False)]

    if not successful_results:
        print("No successful runs!")
        return

    print(f"\nSuccessful runs: {len(successful_results)}/{len(all_results)}\n")

    # Sort by execution time
    successful_results.sort(key=lambda x: x.get('average_time', float('inf')))

    print(f"{'Rank':<6}{'Schedule':<25}{'Partitions':<12}{'Avg Time (s)':<15}{'Triangles':<15}")
    print("-"*80)

    for i, result in enumerate(successful_results, 1):
        schedule = result['schedule']
        partition_scale = result['partition_scale']
        avg_time = result.get('average_time', 'N/A')
        triangles = result.get('triangle_count', 'N/A')
        num_parts = result.get('num_partitions', 'N/A')

        print(f"{i:<6}{schedule:<25}{num_parts:<12}{avg_time:<15}{triangles:<15}")

    print("\nBest configuration:")
    best = successful_results[0]
    print(f"  Schedule: {best['schedule']}")
    print(f"  Partition Scale: {best['partition_scale']}")
    print(f"  Num Partitions: {best.get('num_partitions', 'N/A')}")
    print(f"  Average Time: {best.get('average_time', 'N/A')} seconds")
    print(f"  Triangle Count: {best.get('triangle_count', 'N/A')}")


def main():
    """Main execution function."""
    print("="*80)
    print("Triangle Counting Optimization Script")
    print("="*80)
    print(f"CPP File: {CPP_FILE}")
    print(f"Build Directory: {BENCHMARK_DIR}")
    print(f"Testing {len(OMP_SCHEDULES)} schedules x {len(PARTITION_SCALES)} partition scales")
    print(f"Total configurations: {len(OMP_SCHEDULES) * len(PARTITION_SCALES)}")
    print("="*80)

    # Verify paths
    if not CPP_FILE.exists():
        print(f"ERROR: CPP file not found: {CPP_FILE}")
        return 1

    if not BENCHMARK_DIR.exists():
        print(f"ERROR: Build directory not found: {BENCHMARK_DIR}")
        return 1

    # Backup original file
    original_content = read_cpp_file()
    backup_cpp_file(original_content)

    all_results = []
    total_configs = len(OMP_SCHEDULES) * len(PARTITION_SCALES)
    current_config = 0

    try:
        for schedule in OMP_SCHEDULES:
            for partition_scale in PARTITION_SCALES:
                current_config += 1
                print(f"\n{'='*80}")
                print(f"Configuration {current_config}/{total_configs}")
                print(f"Schedule: {schedule}")
                print(f"Partition Scale: {partition_scale}")
                print(f"{'='*80}")

                # Modify the cpp file
                if not modify_cpp_file(schedule, partition_scale):
                    print("Skipping due to modification failure")
                    continue

                # Build
                build_success = build_benchmark()
                if not build_success:
                    print("Skipping due to build failure")
                    all_results.append({
                        'schedule': schedule,
                        'partition_scale': partition_scale,
                        'build_success': False,
                        'run_success': False
                    })
                    continue

                # Run
                run_success, run_results = run_benchmark()

                # Store results
                result = {
                    'schedule': schedule,
                    'partition_scale': partition_scale,
                    'build_success': build_success,
                    'run_success': run_success,
                    **run_results
                }
                all_results.append(result)

                # Save intermediate results
                save_results(all_results)

                print(f"Configuration {current_config}/{total_configs} completed")

                # Small delay between runs
                time.sleep(2)

        # Print final summary
        print_summary(all_results)

    except KeyboardInterrupt:
        print("\n\nInterrupted by user!")
        save_results(all_results)
        print_summary(all_results)
    finally:
        # Restore original file
        print("\nRestoring original file...")
        restore_cpp_file()
        print("Done!")

    return 0


if __name__ == "__main__":
    sys.exit(main())
