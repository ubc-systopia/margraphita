# Link prediction: references for future benchmarks

Companion to `../random_walk_references.md`. Papers on **candidate generation**
(which pairs to score) and **feature-based / stacked link prediction**, with
notes on how each maps onto FlexoGraph's linkpred bench.

Status legend: ✅ verified (Nov 2026 search / read) · ⚠ from memory · ❌ no code found.
Numbers are the authors' own.

## Contents
1. [Candidate generation](#1-candidate-generation)
2. [Feature-based and stacked link prediction](#2-feature-based-and-stacked-link-prediction)
3. [Feature taxonomy → FlexoGraph](#3-feature-taxonomy--flexograph)
4. [Benchmark idea: stacked feature extraction at scale](#4-benchmark-idea-stacked-feature-extraction-at-scale)
5. [Other related work](#5-other-related-work)

---

## 1. Candidate generation

### LinkWaldo ✅ (most relevant)
- Belth, Büyükçakır, Koutra, *A Hidden Challenge of Link Prediction: Which Pairs to Check?*,
  ICDM 2020, pp. 831–840. [PDF](https://cbelth.github.io/public/assets/documents/belth-2020-ICDM-LinkWaldo.pdf),
  [arXiv 2102.07878](https://arxiv.org/pdf/2102.07878)
- Extended: *Knowledge and Information Systems* 64:743–771, 2022.
  [PDF](https://cbelth.github.io/public/assets/documents/belth-2022-KAIS-LinkWaldo.pdf),
  [Springer](https://link.springer.com/article/10.1007/s10115-021-01632-x)
- Code: [GemsLab/LinkWaldo](https://www.github.com/GemsLab/LinkWaldo)

```bibtex
@inproceedings{belth2020hidden,
  title={A Hidden Challenge of Link Prediction: Which Pairs to Check?},
  author={Belth, Caleb and B{\"u}y{\"u}k{\c{c}}ak{\i}r, Alican and Koutra, Danai},
  booktitle={2020 IEEE International Conference on Data Mining (ICDM)},
  pages={831--840}, year={2020}, organization={IEEE}
}
```

**Setup:** Python, in memory, single machine. `train_test.py` builds static or
temporal splits. `--method` = LinkWaldo | `cn` | `js` | `aa` | `lapm` | `nmf+bag`;
`--embedding_method` = netmf1 | netmf2 | bine | aa; `-DG` degree grouping (default on);
`-tol` / `-b_tol` exact vs approximate search. 13 networks (sizes: ⚠ check paper table).

**Method:**
1. Split the pair space into **equivalence classes** (stochastic-block-model blocks;
   default: nodes grouped by degree): *where* new links tend to fall.
2. Budget candidates across classes in proportion to expected links.
3. Within each class, pick closest pairs by **embedding proximity** using
   **locality-sensitive hashing** (no exhaustive search).

**Claims:** sub-quadratic in nodes, linear in edges; candidate sets contain 7–33% more
missing/future links than embedding and heuristic baselines.

**Key points for FlexoGraph:**
- Candidate generation is its own step, measured by **recall** (not precision).
- Heuristics (CN, Jaccard, AA) are **zero beyond 2 hops**, so FoF gives the same
  top-k as exhaustive for them, but misses links further away.
- FlexoGraph's FoF ≈ its `cn`/`js`/`aa` baselines. Degree grouping needs only degree
  (cheap DB metadata); LSH would build on the embedding-property path.
- Bipartite graphs (user–item) break the 2-hop assumption.

### Recall ceiling of 2-hop candidates ✅
- *Can LLMs Predict Academic Collaboration? Topology Heuristics vs. LLM-Based Link
  Prediction on Real Co-authorship Networks*, 2026. [arXiv 2604.01379](https://arxiv.org/pdf/2604.01379)
- Uses 2-hop candidates; only ~17–21% of new edges fell between pairs sharing a
  collaborator. ~80% are invisible to neighbor-based candidates.

### 2-hop vs 3-hop ✅
- [arXiv 1909.00174](https://arxiv.org/abs/1909.00174v1): study over 128 networks of
  2-hop vs 3-hop indices (the "L3" path-count claim is contested).
- Test with linkpred's `--candidates=khop --k=3`.

### Embedding nearest-pair search ✅
- Pachev & Webb, *Fast link prediction for large networks using spectral embedding*,
  2017. [arXiv 1703.09693](https://arxiv.org/pdf/1703.09693): k-closest pairs in
  embedding space instead of all non-adjacent pairs.

### Recent work
No dedicated 2022–2025 follow-up on candidate selection found (check Google Scholar
"cited by" for the ICDM paper). Most work since HeaRT (below) scores *given* pairs
rather than choosing them. **No candidate-generation work found on a graph DB.**

---

## 2. Feature-based and stacked link prediction

### Meta-learning link predictors ✅ (feature spec + accuracy protocol)
- Singh, Van Kleunen, Clauset (CU Boulder), *Meta-learning optimizes predictions of
  missing links in real-world networks*, arXiv preprint Aug 2025 (not peer reviewed).
  [arXiv 2508.09069](https://arxiv.org/html/2508.09069v1)
- Code: [bis1999/edgepredict](https://github.com/bis1999/edgepredict);
  corpus: [Aghasemian/OptimalLinkPrediction](https://github.com/Aghasemian/OptimalLinkPrediction)
  (from Ghasemian et al.'s stacking paper, ⚠ PNAS 2020).

**Idea:** no single predictor wins everywhere; an XGBoost meta-learner picks
*which* predictor to run from 6 cheap graph stats (mean clustering, mean path length,
degree assortativity, node count, mean degree, degree variance).

**Unsupervised vs supervised:**
- Unsupervised: one pair-level function used alone as a ranking (= linkpred cn/jaccard).
- Supervised ("stacking"): all 42 features → RF / XGBoost / LR / SVM.
  GNNs (GCN, GraphSAGE via PyG) compared alongside.

**Results:** RF stacking wins most often by AUC; LR, GCN, XGB do well by top-k.
Meta-learner within ~0.01 AUC of the best choice. SVM ~140× slower than the fast
group; GNNs 2–4× slower.

**Setup:** Python (NetworkX features, scikit-learn / XGBoost, PyG), in memory.
550 networks averaging **~510 nodes / ~1,155 edges**; largest checks 22k–38k nodes,
≤ ~289k edges (Facebook Page-Page, GitHub Social, Deezer Europe). ~1 s per network.
→ Says nothing about systems performance; useful as a **feature spec** and
**accuracy reference**.

**On the abstract's "scales to large networks" claim** (checked against the paper):
- "Large" = the three SNAP graphs above (≤ 38k nodes, ≤ 289k edges): large for
  the link-prediction corpus, small in systems terms (fits in a few MB).
- No absolute runtimes for them; only relative (full meta-learner 6.7–11× slower
  than SAGE; restricted version 5–29% faster than SAGE).
- No sampling / approximation described for features. Mean geodesic path length
  (a meta-learner input) and betweenness / closeness (stacking inputs) are
  all-pairs shortest-path costs (≈ O(nm) exact): fine at 38k nodes, not at
  millions without approximation.
- Training negatives are subsampled to 10,000 non-edges; the evaluation
  candidate set on the large graphs is not described.
- "Outperforms all state of the art" = vs LR / RF / XGB / SVM stacking and
  GCN / GraphSAGE only. SEAL, BUDDY, NCN, NBFNet are not compared.
→ Scaling to millions of nodes, and the candidate-set question, are open:
exactly where a DB-side feature pipeline could contribute.

### Neo4j GDS link prediction ✅
- Pipeline (beta) with LR, random forest, MLP (`addMLP` is alpha) on pair features
  from embeddings (FastRP, node2vec, GraphSAGE); runs on an in-memory projection;
  needs an undirected graph.
  [ML reference](https://neo4j.com/docs/graph-data-science/current/operations-reference/machine-learning-references/),
  [LP pipelines](https://neo4j.com/docs/graph-data-science/current/algorithms/ml-models/linkprediction/)

### Classifier note
Gradient boosting vs random forest for link prediction: **no direct benchmark found**.
Classifier training is cheap and DB-independent; benchmark feature extraction instead.

---

## 3. Feature taxonomy → FlexoGraph

From the meta-learning paper (42 features):

| Level | Paper features | FlexoGraph equivalent |
|---|---|---|
| Global (8) | nodes, edges, mean degree, degree variance, assortativity, triangles, mean clustering | degree scan; `tc*.cpp`; `cc*.cpp` |
| Pair (14) | **CN, Jaccard** | linkpred `cn` / `jaccard` |
| | **AA, RA, preferential attachment** | small additions to `linkpred_scores.h` (degree-weighted CN; degree product) |
| | shortest path, personalized PageRank, LRA variants | `sssp*.cpp`; random walks (`../random_walk_references.md`) |
| Node (20, ×2 for u and v) | degree, clustering, triangles | degree; `tc*.cpp` |
| | **PageRank**, eigenvector, Katz | `pagerank*.cpp` |
| | **betweenness**, closeness, load | `bc.cpp`, `bc_parallel.cpp` (exact is O(nm); approximate at scale) |

Node-level features are computed **once per node, stored as properties, joined to
pairs**: the write-back / feature-store pattern (`addFeature` idea).

---

## 4. Benchmark idea: stacked feature extraction at scale

| Stage | Work | DB pattern |
|---|---|---|
| Global + node | PageRank, triangles, BC (optional), degree → write node properties | full scans, write-back |
| Candidates | FoF / k-hop / LinkWaldo-style (degree classes + embedding LSH) | traversal |
| Pair | CN, Jaccard, AA, RA, PA + stored node features of u, v | intersections + property lookups |
| Classifier (offline) | RF / XGBoost with edgepredict code | none: AUC / top-k only, to check accuracy |

Report per stage: time, DB reads, peak memory, candidate recall (by distance bucket:
2, 3, ≥4, disconnected), AUC / top-k.

**Baselines:** edgepredict / NetworkX pipeline (where it stops scaling); Neo4j GDS
(projection → features → LR/RF).
**Datasets:** OptimalLinkPrediction corpus for correctness against their numbers;
OGB-sized graphs (ogbl-collab, ogbl-ppa) for scaling.

---

## 4b. Graph DB baselines

No published benchmark compares graph DBs on link prediction or random walks
(searched Nov 2026). Existing DB comparisons (LDBC SNB, Graphalytics, blog suites)
cover queries, ingest, BFS / PR / WCC / CDLP / LCC / SSSP only. So the
comparison has to be built: run the same pairs / graph through each DB's built-in
functions.

| DB | Link prediction support | Storage while running | Link |
|---|---|---|---|
| Neo4j GDS (plugin) | pairwise Cypher functions `gds.alpha.linkprediction.{commonNeighbors, adamicAdar, ...}` (alpha; newer docs `gds.linkprediction.*`); LP pipeline (LR / RF / MLP) | functions: on the store; pipeline: in-memory projection | [Adamic-Adar](https://neo4j.com/docs/graph-data-science/current/algorithms/topological-link-prediction/adamic-adar/) |
| Memgraph MAGE | node2vec embeddings → link prediction; GNN link prediction module | **in-memory DB** | [ML docs](https://memgraph.com/docs/ai-ecosystem/machine-learning), [tutorial repo](https://github.com/memgraph/link-prediction-node-embeddings) |
| TigerGraph GDS library | topological LP in GSQL: Adamic-Adar, preferential attachment, resource allocation, same-community, total neighbors (CN / Jaccard ⚠ not confirmed) | distributed, in-memory-oriented | [docs](https://docs.tigergraph.com/graph-ml/current/intro/), [blog](https://www.tigergraph.com/blog/graph-data-science-library/) |
| ArcadeDB | `algo.adamicAdar` and other LP procedures (⚠ newer, not reviewed) | disk-based multi-model | [docs](https://docs.arcadedb.com/arcadedb/reference/graph-algorithms/chapter) |
| Kùzu | algo extension (WCC, PageRank, Louvain, k-core); no LP found; project reported archived | disk-based embedded | [0.10.0 release](https://blog.kuzudb.com/post/kuzu-0.10.0-release) |

**Suggested set:** Neo4j GDS (functions = closest to per-pair DB scoring; also the
on-store vs projection contrast), Memgraph (in-memory DB ceiling), ArcadeDB
(disk-based peer, verify first). TigerGraph if a licence is available.
**Fairness:** same pairs (export FlexoGraph's FoF / exhaustive candidate list),
same machine and memory cap, report projection / load time separately,
count Cypher round-trip overhead (batch with `UNWIND`).

General DB comparisons for context: [LDBC SNB on Neo4j vs TigerGraph](https://arxiv.org/pdf/1907.07405),
[LDBC Graphalytics](https://github.com/ldbc/ldbc_graphalytics) (LCC ≈ triangle /
clustering features), [graph-bench](https://github.com/tamnd/graph-bench),
[Kùzu study](https://github.com/prrao87/kuzudb-study).

---

## 5. Other related work

| Work | Relevance | Link |
|---|---|---|
| HeaRT, Li et al., NeurIPS 2023 (Datasets & Benchmarks) | baselines were under-tuned; random negatives make evaluation too easy; hard negatives via heuristics | [arXiv 2306.10453](https://arxiv.org/abs/2306.10453v2) |
| BUDDY / ELPH, Chamberlain et al., ICLR 2023 | per-node **sketches** (MinHash / HyperLogLog) replace subgraph extraction; precomputed, storable as node properties | [arXiv 2209.15486](https://arxiv.org/abs/2209.15486v3) |
| NCN / NCNC, Wang et al., ICLR 2024 | common-neighbor-guided GNN; "completion" fills common neighbors missing from incomplete graphs | [arXiv 2302.00890](https://arxiv.org/abs/2302.00890) |
| Gelato, KDD 2025 | sparse similarity ranking for very imbalanced link prediction; GPU, METIS partitions | [PDF](https://cs.rice.edu/~al110/pubs/KDD25.pdf) |
| LPNL, ACL Findings 2024 | LLM-based link prediction on large graphs | [PDF](https://aclanthology.org/2024.findings-acl.215.pdf) |
| Topological feature capability | RF over 21 heuristic indexes; GB / AdaBoost supplement | [arXiv 2206.15101](https://arxiv.org/pdf/2206.15101) |

**Link to walks:** node2vec weights are distance classes (d(prev, x) = 0/1/2), and the
d = 1 class is N(prev) ∩ N(cur), the same intersection CN computes. See
`../random_walk_references.md` §5.
