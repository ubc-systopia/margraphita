#include "ml_sampling.h"
#include <algorithm>
#include <memory>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace ml_sampling {
namespace {
struct CursorCloser {
    template<class T> void operator()(T* p) const noexcept {
        if (p) { p->close(); delete p; }
    }
};
template<class Cursor>
std::vector<node_id_t> neighbors(Cursor& cursor, node_id_t id) {
    cursor.reset();
    cursor.set_key_range({id, id});
    adjlist row{};
    cursor.next(&row);
    if (row.node_id != id) return {};
    auto& ids = row.edgelist;
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return std::move(ids);
}
// Unbiased bounded draw with reproducible mt19937_64 output.
std::size_t draw(std::mt19937_64& rng, std::size_t bound) {
    const auto b = static_cast<std::uint64_t>(bound);
    const auto threshold = (std::uint64_t(0) - b) % b;
    std::uint64_t x;
    do { x = rng(); } while (x < threshold);
    return static_cast<std::size_t>(x % b);
}
std::vector<node_id_t> choose(std::vector<node_id_t> ids, int fanout,
                              bool replacement, std::mt19937_64& rng) {
    if (ids.empty() || fanout == 0) return {};
    if (fanout == -1) return ids;
    const auto k = static_cast<std::size_t>(fanout);
    if (replacement) {
        std::vector<node_id_t> result;
        result.reserve(k);
        for (std::size_t i=0; i<k; ++i) result.push_back(ids[draw(rng, ids.size())]);
        return result;
    }
    if (k >= ids.size()) return ids;
    for (std::size_t i=0; i<k; ++i)
        std::swap(ids[i], ids[i + draw(rng, ids.size()-i)]);
    ids.resize(k);
    return ids;
}
}
SampledGraph sample_neighbors(GraphBase& graph,
    const std::vector<node_id_t>& seeds, const SamplingOptions& options) {
    for (int f : options.fanouts)
        if (f < -1) throw std::invalid_argument("fanout must be >= -1");
    if (options.direction != Direction::Incoming && options.direction != Direction::Outgoing)
        throw std::invalid_argument("invalid sampling direction");
    SampledGraph result;
    std::unordered_map<node_id_t, std::size_t> index;
    auto intern = [&](node_id_t id) {
        auto entry = index.emplace(id, result.node_ids.size());
        if (entry.second) result.node_ids.push_back(id);
        return entry.first->second;
    };
    std::vector<std::size_t> frontier;
    for (auto id : seeds) {
        if (index.find(id) == index.end()) {
            if (id == OutOfBand_ID_MAX || !(graph.is_read_optimized() ?
                graph.get_node(id).id == id : graph.has_node(id)))
                throw std::invalid_argument("seed vertex does not exist");
            frontier.push_back(intern(id));
        }
        result.seed_indices.push_back(index.at(id));
    }
    std::unique_ptr<InCursor, CursorCloser> in;
    std::unique_ptr<OutCursor, CursorCloser> out;
    std::mt19937_64 rng(options.seed);
    for (int f : options.fanouts) {
        SampledHop hop;
        hop.frontier = frontier;
        std::vector<std::size_t> next;
        std::unordered_set<std::size_t> seen;
        if (f != 0 && !frontier.empty()) {
            if (options.direction == Direction::Incoming && !in) in.reset(graph.get_innbd_iter());
            if (options.direction == Direction::Outgoing && !out) out.reset(graph.get_outnbd_iter());
            for (auto local : frontier) {
                const auto id = result.node_ids[local];
                auto ids = in ? neighbors(*in, id) : neighbors(*out, id);
                for (auto neighbor : choose(std::move(ids), f, options.replacement, rng)) {
                    const auto n = intern(neighbor);
                    SampledEdge edge = in ? SampledEdge{n,local} : SampledEdge{local,n};
                    hop.edges.push_back(edge);
                    result.edges.push_back(edge);
                    if (seen.insert(n).second) next.push_back(n);
                }
            }
        }
        result.hops.push_back(std::move(hop));
        frontier = std::move(next);
    }
    return result;
}
}
