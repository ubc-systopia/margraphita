// Listing the various ways of candidate generation

// Exhaustive and trival baselines:
//      Exhaustive - All pairs: score every non-edge pair. This is O(n²) and only works on small graphs.
//      Uniform random sampling: draw random non-edges. It's mostly used to make training negatives or for OGB-style evaluation (e.g., ogbl-citation2 ranks each positive against 1,000 random negatives). It has poor recall as a real generator.
//Locality / k-hop neighborhood
//      Distance-bounded candidates: only pairs within k hops, usually 2 or 3.
//      Friends-of-friends / triadic closure: close open triangles. This is the classic "People You May Know" generator used at Facebook and LinkedIn.
//      Ego-net / circle-of-trust expansion: take a user's top neighbors and expand from them. Twitter's Who-To-Follow does this.
//      Meta-path-constrained neighborhoods in heterogeneous graphs: candidates are nodes reachable along a typed path such as A–P–A. PathSim and meta-path walks work this way.


#include <iostream>
#include <vector>
#include <utility>
#include <queue>
#include <random>

using Graph         = std::vector<std::vector<int>>;
using CandidateList = std::vector<std::pair<int, int>>;

// helper
static bool is_edge(const Graph& g, int u, int v) {
    for (int w : g[u]) if (w == v) return true;
    return false;
}

// ---------------------------------------------------------------
// Exhaustive - all pairs
// ---------------------------------------------------------------
// Node picking:
//   Take every node u in order. Pair it only with the nodes that
//   come after it in the ordering (u+1, u+2, ... end) so each
//   unordered pair is looked at once. If the pair is not already
//   an edge, it becomes a candidate.
// Cost is O(n²); only runs on small graphs.
//
// Two flavors:
//   exhaustive_all_pairs_each  — LAZY.  Calls emit(c) per candidate.
//                                Return false from emit to stop early.
//   exhaustive_all_pairs       — EAGER. Builds the full list, returns it.
//                                Just the lazy one with a "collect" emit.
template <typename F>
void exhaustive_all_pairs_each(const Graph& g, F emit) {
    int n = (int)g.size();
    for (int u = 0; u < n; ++u)
        for (int v = u + 1; v < n; ++v)
            if (!is_edge(g, u, v) && !emit(std::pair<int,int>{u, v})) return;
}

CandidateList exhaustive_all_pairs(const Graph& g) {
    CandidateList out;
    exhaustive_all_pairs_each(g, [&](std::pair<int,int> c) {
        out.push_back(c); return true;
    });
    return out;
}

// ---------------------------------------------------------------
// Uniform random sampling
// ---------------------------------------------------------------
// Node picking:
//   Repeatedly draw two nodes u and v independently, uniformly at
//   random over all nodes. Throw the draw away if u == v or if the
//   pair is already an edge. Otherwise emit it. Stop after k draws.
// Mostly a source of negatives / eval candidates, not a real generator.
template <typename F>
void uniform_random_sampling_each(const Graph& g, int k, unsigned seed, F emit) {
    int n = (int)g.size();
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> pick(0, n - 1);
    for (int i = 0; i < k; ++i) {
        int u = pick(rng);
        int v = pick(rng);
        if (u == v)          continue;
        if (is_edge(g, u, v)) continue;
        if (!emit(std::pair<int,int>{u, v})) return;
    }
}

CandidateList uniform_random_sampling(const Graph& g, int k, unsigned seed) {
    CandidateList out;
    uniform_random_sampling_each(g, k, seed, [&](std::pair<int,int> c) {
        out.push_back(c); return true;
    });
    return out;
}

// ---------------------------------------------------------------
// Distance-bounded candidates (k-hop)
// ---------------------------------------------------------------
// Node picking:
//   Pick each node u as a source and BFS outward from it up to k hops
//   (usually 2 or 3). Every node v reached within k hops, that is not
//   u itself and not already an edge with u, becomes a candidate.
//   To avoid emitting each pair twice, only keep the pair when u < v.
//
//   Note: "within k hops" includes dist 1 (existing edges) — those are
//   filtered out by the >= 2 check below, so with k=2 you get exactly
//   the 2-hop pairs, with k=3 you also get 3-hop pairs, etc.
template <typename F>
void distance_bounded_candidates_each(const Graph& g, int k, F emit) {
    int n = (int)g.size();
    for (int u = 0; u < n; ++u) {
        // BFS from u up to depth k
        std::vector<int> dist(n, -1);
        std::queue<int> q;
        dist[u] = 0;
        q.push(u);
        while (!q.empty()) {
            int x = q.front(); q.pop();
            if (dist[x] == k) continue;          // don't expand beyond k
            for (int y : g[x]) {
                if (dist[y] == -1) {
                    dist[y] = dist[x] + 1;
                    q.push(y);
                }
            }
        }
        // emit pairs (u, v) with u < v and 2 <= dist[v] <= k
        for (int v = u + 1; v < n; ++v) {
            if (dist[v] >= 2) {
                if (!emit(std::pair<int,int>{u, v})) return;
            }
        }
    }
}

CandidateList distance_bounded_candidates(const Graph& g, int k = 2) {
    CandidateList out;
    distance_bounded_candidates_each(g, k, [&](std::pair<int,int> c) {
        out.push_back(c); return true;
    });
    return out;
}


// ---------------------------------------------------------------
// Friends-of-friends / triadic closure
// ---------------------------------------------------------------
// Node picking:
//   For each node u, look at its direct neighbors (one hop), then at
//   each neighbor's neighbors (two hops) to reach the friends-of-
//   friends set. For each v in that set: skip v if it's u, skip it if
//   it's already a direct neighbor of u (i.e. the triangle is closed),
//   and only emit when u < v to dedup. What's left are open triangles,
//   i.e. "People You May Know".
CandidateList friends_of_friends(const Graph& g);


// ---------------------------------------------------------------
// Ego-net / circle-of-trust expansion
// ---------------------------------------------------------------
// Node picking:
//   For each user u, first pick u's inner circle: its top-T neighbors
//   ranked by edge weight or score. Then expand from that circle by
//   walking each inner-circle node's neighbors (one hop out from the
//   circle). Any node v found that way, that is not u and not already
//   a direct neighbor of u, is a candidate. Dedup with u < v.
// Same idea as Twitter's Who-To-Follow.
CandidateList ego_net_expansion(const Graph& g, int T = 20);


// ---------------------------------------------------------------
// Meta-path constrained neighborhoods (heterogeneous graphs)
// ---------------------------------------------------------------
// Node picking:
//   Nodes are typed. Starting from each source node of the first type
//   in the meta-path (e.g. A), walk along the typed path step by step
//   (A -> P -> A -> ...). The endpoints of those walks are the
//   candidates. Skip u itself and already-connected pairs; dedup with
//   u < v. This is the PathSim / meta-path-walk pattern.
CandidateList meta_path_candidates(const Graph& g,
                                   const std::vector<int>& node_type,
                                   const std::vector<int>& type_path);