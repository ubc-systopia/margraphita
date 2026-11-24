#pragma once
#include "graph.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace ml_access {
enum class RetrievalStrategy { Direct, Sorted, SortedDeduplicated };

// Raw dense vectors must be explicitly known to contain native float32 values.
struct VectorRequest {
    std::size_t dimension;
    RetrievalStrategy strategy = RetrievalStrategy::Direct;
};

// Column indices are relative to the selected property cursor's value format.
// These are the two types currently exposed by NodePropCursor.
enum class AttributeType { UInt64, Int32 };
struct AttributeColumn {
    int index;
    AttributeType type;
};
struct AttributeRequest {
    std::string table;
    std::string colgroup;
    std::vector<AttributeColumn> columns;
    RetrievalStrategy strategy = RetrievalStrategy::Direct;
};
using AttributeValue = std::variant<std::uint64_t, std::int32_t>;

// Row-major owned output, preserving input order and duplicates.
struct VectorBatch {
    std::size_t rows = 0;
    std::size_t dimension = 0;
    std::vector<float> values;
};
struct AttributeBatch {
    std::size_t rows = 0;
    std::size_t columns = 0;
    std::vector<AttributeValue> values;
};
} // namespace ml_access

namespace ml_access {
// Only use vectors for properties known to contain native float32 arrays.
// GraphBase handle is borrowed; calls must not share it across threads.
VectorBatch get_vertex_vectors(GraphBase& graph,
    const std::vector<node_id_t>& ids, const VectorRequest& request);
// Caller supplies schema-validated column indices and types.
AttributeBatch get_vertex_attributes(GraphBase& graph,
    const std::vector<node_id_t>& ids, const AttributeRequest& request);
}

namespace ml_access {
struct EdgeId {
    node_id_t src;
    node_id_t dst;
    bool operator<(const EdgeId& other) const {
        return src < other.src || (src == other.src && dst < other.dst);
    }
    bool operator==(const EdgeId& other) const {
        return src == other.src && dst == other.dst;
    }
};
// Dense edge blobs must contain native float32 arrays.
VectorBatch get_edge_vectors(GraphBase& graph,
    const std::vector<EdgeId>& ids, const VectorRequest& request);
// EdgePropCursor currently supports UInt64 columns only.
AttributeBatch get_edge_attributes(GraphBase& graph,
    const std::vector<EdgeId>& ids, const AttributeRequest& request);
}

namespace ml_access {
// Could be called write_vertex_embeddings() if limited to embeddings.
// Use vectors instead to support features and intermediate representations too.
// Replaces existing blobs; caller must reserve them for raw Float32 vectors.
// Direct writes in input order; last duplicate wins. Not an atomic batch.
// Storage contract: the caller must ensure the database stores vertex properties as raw
// Float32 blobs (EMBEDDED-style property storage). The ML API cannot inspect the graph's
// storage configuration (GraphBase keeps its options protected), so it does not check this.
// A GraphBase supports_embedded_vertex_vectors() / supports_embedded_edge_vectors() could
// be added in the future to reject incompatible storage before writing.
void set_vertex_vectors(                       // Store vertex vectors
    GraphBase& graph,                          // Existing graph handle
    const std::vector<node_id_t>& ids,          // Destination vertex IDs
    const VectorBatch& batch);                 // Row-major Float32 data

// Same operation for edges; could be called write_edge_embeddings().
// Same storage contract: the caller must ensure raw Float32 edge property blobs.
void set_edge_vectors(                         // Store edge vectors
    GraphBase& graph,                          // Existing graph handle
    const std::vector<EdgeId>& ids,             // Source/destination pairs
    const VectorBatch& batch);                 // Row-major Float32 data
}
