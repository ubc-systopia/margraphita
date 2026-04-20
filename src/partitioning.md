# FlexoGraph Partitioning Strategies

## Overview

FlexoGraph uses a configurable partitioning strategy (graph_engine.cpp:108-122) selected via the `partition_strategy` field in GraphEngine. The strategy determines how graph nodes are divided among threads for parallel processing.

## Strategy Selection

The partitioning strategy is chosen in `create_ro_graph_handle()` using a switch statement:

```cpp
switch(partition_strategy) {
  case PartitionStrategy::NODE_COUNT:
    opts.num_nodes = compute_nodes_and_partition(num_threads, ptr);
    break;
  case PartitionStrategy::EDGE_AWARE:
    opts.num_nodes = new_parts(num_threads, ptr, partition_scale);
    break;
  case PartitionStrategy::NODE_COUNT_FINE:
    opts.num_nodes = make_min_parts(num_threads, ptr, partition_scale);
    break;
  default:
    throw GraphException("Invalid partition strategy");
}
```

The `partition_scale` parameter controls the granularity for strategies that support it.

## Comparison of the Three Partitioning Strategies

| Aspect                     | NODE_COUNT (compute_nodes_and_partition)                | EDGE_AWARE (new_parts)                                           | NODE_COUNT_FINE (make_min_parts)                                          |
|----------------------------|---------------------------------------------------------|------------------------------------------------------------------|---------------------------------------------------------------------------|
| Balancing Strategy         | Node-count based                                        | Edge/degree based                                                | Node-count based with fine granularity                                    |
| What it considers          | Number of nodes only                                    | Outdegree of each node                                           | Number of nodes only                                                      |
| Partition count            | Exactly thread_max                                      | thread_max * partition_scale (variable)                          | thread_max * partition_scale (fixed)                                      |
| How partitions are created | Divides total nodes by threads, creates fixed intervals | Accumulates outdegrees, creates partition when threshold reached | Divides total nodes by target partitions, creates fixed intervals         |
| Best for                   | Graphs with uniform degree distribution                 | Graphs with skewed/non-uniform degree distribution               | Uniform graphs needing fine-grained dynamic scheduling                    |
| Complexity                 | Single pass, O(n)                                       | Single pass, O(n)                                                | Two passes, O(n)                                                          |
| Number of passes           | 1 (collect + partition)                                 | 1 (collect + partition simultaneously)                           | 2 (first collect all, then partition)                                     |
| Populates all_node_ids     | No                                                      | Yes                                                              | Yes                                                                       |

## Detailed Breakdown

### 1. NODE_COUNT: compute_nodes_and_partition() (graph_engine.cpp:206-277)

**Goal:** Simple node-count balanced partitioning

**Algorithm:**
- Collects all node IDs in a single pass
- Divides nodes equally: `nodes_per_partition = ceil(total_nodes / thread_max)`
- Creates exactly `thread_max` partitions with approximately equal node counts
- Stores partition boundaries in `node_ranges` vector

**Pros:**
- Simple, predictable partition size
- Fast single-pass implementation
- Works well when node work is uniform

**Cons:**
- Ignores degree imbalance
- Doesn't account for workload differences between nodes
- May cause load imbalance in skewed graphs

**Use when:** Graph has uniform degree distribution and you want simple, equal-sized partitions.

### 2. EDGE_AWARE: new_parts() (graph_engine.cpp:292-386) ⭐

**Goal:** Balance workload by considering edge distribution

**Algorithm:**
- Calculates target: `edges_per_thread = total_edges / (thread_max * partition_scale)`
- Iterates through nodes, accumulating their outdegrees
- Creates a new partition boundary when accumulated edges reach threshold
- Creates variable number of partitions based on actual degree distribution
- Populates `all_node_ids` for use with `get_work_chunks()`

**Pros:**
- Accounts for non-uniform degree distributions
- Better load balancing for skewed graphs (e.g., power-law, scale-free)
- Each partition has approximately equal edge work

**Cons:**
- More complex logic
- Requires knowing total edge count upfront
- Variable partition count may be less than `thread_max * partition_scale` for sparse graphs

**Use when:** Graph has skewed degree distribution (social networks, web graphs, power-law graphs).

### 3. NODE_COUNT_FINE: make_min_parts() (graph_engine.cpp:388-478)

**Goal:** Node-count balanced partitioning with finer granularity

**Algorithm:**
- First pass: Collects all node IDs
- Calculates target: `target_partitions = thread_max * partition_scale`
- Calculates: `nodes_per_partition = ceil(total_nodes / target_partitions)`
- Creates `target_partitions` partitions at fixed node count intervals
- Populates `all_node_ids` for use with `get_work_chunks()`

**Pros:**
- Provides more partitions than threads for better dynamic scheduling
- Simple to reason about (uniform node counts per partition)
- Enables finer-grained work stealing/load balancing

**Cons:**
- Ignores degree imbalance like NODE_COUNT
- Two-pass approach (slightly slower than single-pass)
- May create many small partitions if `partition_scale` is large

**Use when:** Need fine-grained control for dynamic scheduling, but degree distribution is relatively uniform.

## Key Differences Summary

1. **Edge-awareness:** EDGE_AWARE (new_parts) is the ONLY strategy that uses edge information for balancing
2. **Partition granularity:**
   - NODE_COUNT: Exactly `thread_max` partitions
   - EDGE_AWARE: Variable (depends on degree distribution), target is `thread_max * partition_scale`
   - NODE_COUNT_FINE: Exactly `thread_max * partition_scale` partitions
3. **Use cases:**
   - Skewed graphs (power-law degree) → Use EDGE_AWARE
   - Uniform graphs → Use NODE_COUNT
   - Uniform graphs needing fine-grained work stealing → Use NODE_COUNT_FINE

## Work Chunks for Dynamic Scheduling

The `get_work_chunks(int chunk_size)` function (graph_engine.cpp:673-703) creates fine-grained work units for dynamic scheduling. It requires `all_node_ids` to be populated, which happens in:
- EDGE_AWARE (new_parts)
- NODE_COUNT_FINE (make_min_parts)

If using NODE_COUNT strategy, `all_node_ids` will be empty and `get_work_chunks()` will throw an exception.

## Configuration

To select a partitioning strategy, set the `partition_strategy` field in GraphEngine:
- `PartitionStrategy::NODE_COUNT` - Simple node-count balancing
- `PartitionStrategy::EDGE_AWARE` - Degree-aware balancing (default for most benchmarks)
- `PartitionStrategy::NODE_COUNT_FINE` - Fine-grained node-count balancing

The `partition_scale` parameter controls granularity for EDGE_AWARE and NODE_COUNT_FINE strategies (typically 1-4).