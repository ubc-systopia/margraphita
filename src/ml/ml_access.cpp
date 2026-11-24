#include "ml_access.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>

namespace ml_access {
namespace {
std::size_t checked_size(std::size_t rows, std::size_t cols) {
    if (!cols || rows > std::numeric_limits<std::size_t>::max() / cols)
        throw std::invalid_argument("Invalid batch dimensions");
    return rows * cols;
}

// Vertex existence. On read-optimized DBs, look the id up in the node table via get_node():
// has_node() only searches adjlistout, so on directed bulk-loaded DBs it misses vertices that
// have in-edges but no out-edges. Otherwise fall back to has_node() (the node table carries no
// data then, and get_node() searches adjlistout too). No core engine change involved.
bool vertex_exists(GraphBase& graph, node_id_t id) {
    if (graph.is_read_optimized())
        return graph.get_node(id).id == id;
    return graph.has_node(id);
}

template<class Id, class T, class Fetch>
void gather(const std::vector<Id>& ids, std::size_t width,
            RetrievalStrategy strategy, std::vector<T>& values, Fetch fetch) {
    std::vector<std::size_t> order(ids.size());
    std::iota(order.begin(), order.end(), 0);
    if (strategy != RetrievalStrategy::Direct)
        std::stable_sort(order.begin(), order.end(), [&](auto a, auto b) {
            return ids[a] < ids[b];
        });
    for (std::size_t i = 0; i < order.size(); ++i) {
        const auto row = order[i];
        auto* dst = values.data() + row * width;
        if (strategy == RetrievalStrategy::SortedDeduplicated && i &&
            ids[row] == ids[order[i-1]]) {
            std::copy_n(values.data() + order[i-1] * width, width, dst);
        } else {
            fetch(ids[row], dst);
        }
    }
}
}

VectorBatch get_vertex_vectors(GraphBase& graph,
    const std::vector<node_id_t>& ids, const VectorRequest& request) {
    static_assert(sizeof(float) == 4, "float32 required");
    const auto dim = request.dimension;
    const auto bytes = checked_size(dim, sizeof(float));
    VectorBatch result{ids.size(), dim, {}};
    result.values.resize(checked_size(ids.size(), dim));
    gather(ids, dim, request.strategy, result.values, [&](node_id_t id, float* dst) {
        if (!vertex_exists(graph, id)) throw std::runtime_error("Unknown vertex: " + std::to_string(id));
        const auto blob = graph.get_node_properties(id);
        // Archived AdjList and SplitEdgeKey return new[] allocations.
        std::unique_ptr<const uint8_t[]> owner(blob.data);
        if (!blob.data || blob.size != bytes)
            throw std::runtime_error("Missing or invalid vector: " + std::to_string(id));
        std::memcpy(dst, blob.data, bytes);
    });
    return result;
}

AttributeBatch get_vertex_attributes(GraphBase& graph,
    const std::vector<node_id_t>& ids, const AttributeRequest& request) {
    const auto width = request.columns.size();
    AttributeBatch result{ids.size(), width, {}};
    result.values.resize(checked_size(ids.size(), width));
    for (const auto& col : request.columns)
        if (col.index < 0) throw std::invalid_argument("Negative column index");
    if (ids.empty()) return result;
    auto cursor = graph.get_node_prop_cursor(request.table, request.colgroup);
    if (!cursor) throw std::runtime_error("Property cursor unavailable");
    gather(ids, width, request.strategy, result.values, [&](node_id_t id, AttributeValue* dst) {
        if (!vertex_exists(graph, id) || !cursor->seek(id))
            throw std::runtime_error("Missing vertex or attributes: " + std::to_string(id));
        for (std::size_t j = 0; j < width; ++j) {
            const auto& col = request.columns[j];
            switch (col.type) {
                case AttributeType::UInt64: dst[j] = cursor->get_uint64(col.index); break;
                case AttributeType::Int32: dst[j] = cursor->get_int32(col.index); break;
                default: throw std::invalid_argument("Unsupported attribute type");
            }
        }
    });
    return result;
}
}

namespace ml_access {
VectorBatch get_edge_vectors(GraphBase& graph,
    const std::vector<EdgeId>& ids, const VectorRequest& request) {
    static_assert(sizeof(float) == 4, "float32 required");
    const auto dim = request.dimension;
    const auto bytes = checked_size(dim, sizeof(float));
    VectorBatch result{ids.size(), dim, {}};
    result.values.resize(checked_size(ids.size(), dim));
    gather(ids, dim, request.strategy, result.values, [&](EdgeId id, float* dst) {
        if (!graph.has_edge(id.src, id.dst))
            throw std::runtime_error("Unknown edge");
        const auto blob = graph.get_edge_properties(id.src, id.dst);
        // AdjList and SplitEdgeKey return new[] allocations.
        std::unique_ptr<const uint8_t[]> owner(blob.data);
        if (!blob.data || blob.size != bytes)
            throw std::runtime_error("Missing or invalid edge vector");
        std::memcpy(dst, blob.data, bytes);
    });
    return result;
}

AttributeBatch get_edge_attributes(GraphBase& graph,
    const std::vector<EdgeId>& ids, const AttributeRequest& request) {
    const auto width = request.columns.size();
    AttributeBatch result{ids.size(), width, {}};
    result.values.resize(checked_size(ids.size(), width));
    for (const auto& col : request.columns)
        if (col.index < 0 || col.type != AttributeType::UInt64)
            throw std::invalid_argument("Edge cursor requires UInt64 columns and nonnegative indices");
    if (ids.empty()) return result;
    auto cursor = graph.get_edge_prop_cursor(request.table, request.colgroup);
    if (!cursor) throw std::runtime_error("Edge property cursor unavailable");
    gather(ids, width, request.strategy, result.values, [&](EdgeId id, AttributeValue* dst) {
        if (!graph.has_edge(id.src, id.dst) || !cursor->seek(id.src, id.dst))
            throw std::runtime_error("Missing edge or attributes");
        for (std::size_t j = 0; j < width; ++j)
            dst[j] = cursor->get_uint64(request.columns[j].index);
    });
    return result;
}
}

namespace ml_access {
namespace {
std::size_t validate_write(const VectorBatch& batch, std::size_t count) {
    static_assert(sizeof(float) == 4, "float32 required"); // Require four-byte floats
    if (batch.rows != count)                              // One vector per ID
        throw std::invalid_argument("Row count mismatch"); // Reject mismatched IDs
    const auto size = checked_size(count, batch.dimension); // Check dimension and overflow
    if (batch.values.size() != size)                      // Check actual float count
        throw std::invalid_argument("Vector size mismatch"); // Reject malformed data
    return checked_size(batch.dimension, sizeof(float));  // Bytes per vector
}
}

void set_vertex_vectors(GraphBase& graph,
    const std::vector<node_id_t>& ids,                        // Destination vertices
    const VectorBatch& batch) {                               // Input vectors
    const auto bytes = validate_write(batch, ids.size());     // Validate shape first
    if (ids.empty()) return;
    // No storage-mode check: the caller must ensure raw Float32 vertex property blobs (see header).
    for (auto id : ids)                                       // Check every ID before writing
        if (!vertex_exists(graph, id))
            throw std::runtime_error("Unknown vertex");
    for (std::size_t i = 0; i < ids.size(); ++i) {              // Process in input order
        const float* row = batch.values.data() + i * batch.dimension; // Start of row i
        graph.set_node_properties(ids[i],                     // Select destination vertex
            reinterpret_cast<const uint8_t*>(row), bytes);    // Store raw bytes using existing API
    }
}

void set_edge_vectors(GraphBase& graph,
    const std::vector<EdgeId>& ids,                           // Destination edges
    const VectorBatch& batch) {                               // Input vectors
    const auto bytes = validate_write(batch, ids.size());     // Validate shape first
    if (ids.empty()) return;
    // No storage-mode check: the caller must ensure raw Float32 edge property blobs (see header).
    for (const auto& id : ids)                                // Check every edge before writing
        if (!graph.has_edge(id.src, id.dst))
            throw std::runtime_error("Unknown edge");
    for (std::size_t i = 0; i < ids.size(); ++i) {              // Process in input order
        const float* row = batch.values.data() + i * batch.dimension; // Start of row i
        graph.set_edge_properties(ids[i].src, ids[i].dst,       // Select destination edge
            reinterpret_cast<const uint8_t*>(row), bytes);    // Store raw bytes using existing API
    }
}
}
