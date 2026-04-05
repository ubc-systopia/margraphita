#pragma once

// snb_prop_spool.h — binary spool files for deferred property writes.
//
// Properties cannot be written during edge loading (SplitEdgeKey requires
// set_node_properties to be called AFTER all add_edge calls).  Instead of
// accumulating all blobs in RAM (the old loader's bottleneck), each vertex/edge
// loader streams its property records to a small binary spool file.  The flush
// phase then reads each spool sequentially and issues set_node/edge_properties
// calls — O(1) RAM per record at all times.
//
// Node prop record  (12 + size bytes):
//   [uint64_t typed_id][uint32_t size][uint8_t data[size]]
//
// Edge prop record  (20 + size bytes):
//   [uint64_t src][uint64_t dst][uint32_t size][uint8_t data[size]]

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "common_defs.h"

// ---- Node prop spool writer ------------------------------------------------

class NodePropSpool {
public:
    explicit NodePropSpool(const std::string& path)
        : path_(path), out_(path, std::ios::binary | std::ios::trunc)
    {
        if (!out_.is_open())
            throw std::runtime_error("NodePropSpool: cannot open " + path);
    }

    void write(node_id_t id, const uint8_t* data, uint32_t size) {
        out_.write(reinterpret_cast<const char*>(&id),   sizeof(id));
        out_.write(reinterpret_cast<const char*>(&size), sizeof(size));
        out_.write(reinterpret_cast<const char*>(data),  size);
    }

    void close()                    { out_.close(); }
    const std::string& path() const { return path_; }

private:
    std::string   path_;
    std::ofstream out_;
};

// ---- Node prop spool reader ------------------------------------------------

class NodePropSpoolReader {
public:
    // Silently no-op if the file does not exist (vertex type may have no props).
    explicit NodePropSpoolReader(const std::string& path)
        : in_(path, std::ios::binary) {}

    // Returns false at EOF or if the file was not opened.
    bool read(node_id_t& id, std::vector<uint8_t>& data) {
        if (!in_.is_open()) return false;
        if (!in_.read(reinterpret_cast<char*>(&id), sizeof(id))) return false;
        uint32_t sz = 0;
        if (!in_.read(reinterpret_cast<char*>(&sz), sizeof(sz))) return false;
        data.resize(sz);
        return static_cast<bool>(
            in_.read(reinterpret_cast<char*>(data.data()), sz));
    }

private:
    std::ifstream in_;
};

// ---- Edge prop spool writer ------------------------------------------------

class EdgePropSpool {
public:
    explicit EdgePropSpool(const std::string& path)
        : path_(path), out_(path, std::ios::binary | std::ios::trunc)
    {
        if (!out_.is_open())
            throw std::runtime_error("EdgePropSpool: cannot open " + path);
    }

    void write(node_id_t src, node_id_t dst, const uint8_t* data, uint32_t size) {
        out_.write(reinterpret_cast<const char*>(&src),  sizeof(src));
        out_.write(reinterpret_cast<const char*>(&dst),  sizeof(dst));
        out_.write(reinterpret_cast<const char*>(&size), sizeof(size));
        out_.write(reinterpret_cast<const char*>(data),  size);
    }

    void close()                    { out_.close(); }
    const std::string& path() const { return path_; }

private:
    std::string   path_;
    std::ofstream out_;
};

// ---- Edge prop spool reader ------------------------------------------------

class EdgePropSpoolReader {
public:
    explicit EdgePropSpoolReader(const std::string& path)
        : in_(path, std::ios::binary) {}

    bool read(node_id_t& src, node_id_t& dst, std::vector<uint8_t>& data) {
        if (!in_.is_open()) return false;
        if (!in_.read(reinterpret_cast<char*>(&src), sizeof(src))) return false;
        if (!in_.read(reinterpret_cast<char*>(&dst), sizeof(dst))) return false;
        uint32_t sz = 0;
        if (!in_.read(reinterpret_cast<char*>(&sz), sizeof(sz))) return false;
        data.resize(sz);
        return static_cast<bool>(
            in_.read(reinterpret_cast<char*>(data.data()), sz));
    }

private:
    std::ifstream in_;
};
