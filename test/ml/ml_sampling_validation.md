# ml_sampling validation (2026-11-24)

## 1. Sampler correctness

`ml_sampling.cpp` unchanged; no sampling bug found.

| test | result |
|---|---|
| `test_ml_sampling` (synthetic, relabelled so every edge has src < dst) | pass |
| web-Stanford, directed, bulk-loaded (scratch copy `scratch/hasnode/adj_rd_web-Stanford`, read-only) | 500 seeds (300 random, 100 sinks, 100 in-degree-0), Incoming + Outgoing: 0 failures |
| s12, undirected (scratch copy `scratch/s12_e8/adj_r_s12_e8`, read-only) | 400 seeds (incl. 109 isolated), both directions: 0 failures |

Per seed and direction the probe checks, against the stored adjacency rows:
1-hop full fanout = the stored neighbour set; 2-hop full fanout = reference edge multiset; fanout 5 without
replacement = min(5, deg) distinct true neighbours; with replacement = 5 true neighbours (0 if deg 0);
same seed = same sample; batch of all seeds: `seed_indices` map back to the seeds and `node_ids` are unique.
On undirected DBs Incoming reads the same neighbours as Outgoing.

Probe: `~/flexography-clone-structural/ml_check/probe_sampling_db.cpp <dir> <name> <directed> [vertex]` (outside the repo).

Reference caveat (engine, not sampler): `AdjList::get_in_nodes_id` returns `{}` for every sink, because it starts with
`has_node()`, which only checks `adjlistout`. On web-Stanford that is 172 vertices whose stored `adjlistin` row and
node-table `in_degree` are non-empty. The sampler reads the in-adjlist row directly, so it is correct for them; a test
that uses `get_in_nodes_id` as the reference reports false failures there.

## 2. OrderNodes limitation (build option, separate from the sampler)

This build has `OrderNodes=True` (`build/release/CMakeCache.txt`). The only code it changes is `AdjList::add_edge`
(`src/adj_list.cpp`): the adjacency entry is written as min(src,dst) -> max(src,dst), in adjlistout and adjlistin.
Degree increments in the node table are computed from the original src/dst before the swap (code reading, not tested).

- Graphs built with `add_edge` (unit tests, API inserts): a directed edge with src > dst is stored reversed.
  The original sampling test inserted 50->1 and 50->7; the DB held 1->50 and 7->50.
- Bulk-loaded graphs: not affected. The loaders (`preprocess/bulk_insert*.cpp`) write the tables directly and never
  call `add_edge`. web-Stanford keeps 1,131,368 of its 2,312,497 out-edges with src > dst, and every node-table degree
  matches its adjacency rows.
- Undirected graphs: no effect on results (both directions are stored).

So directed ML tests that build graphs through `add_edge` must use src < dst edges, or a build without OrderNodes.
