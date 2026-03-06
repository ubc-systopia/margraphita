#ifndef PROP_SCHEMA_H
#define PROP_SCHEMA_H

#include <cstdint>
#include <cstring>

// ============================================================
// Property schemas for LDBC SNB PoC subset:
//   Vertices: Person, Post
//   Edges:    knows, hasCreator (no props), likes
// Header-only, no dependencies on Flexograph types.
// ============================================================

// --- Person vertex schema ---
// | creationDate (int64) | birthday (int64) | gender (int8) |
// | offset 0             | offset 8         | offset 16     |
// Total: 17 bytes

namespace SNBPersonSchema {
    constexpr size_t OFFSET_CREATION_DATE = 0;
    constexpr size_t OFFSET_BIRTHDAY      = 8;
    constexpr size_t OFFSET_GENDER        = 16;
    constexpr size_t TOTAL_SIZE           = 17;

    inline void set_creation_date(uint8_t* buf, int64_t val) {
        std::memcpy(buf + OFFSET_CREATION_DATE, &val, sizeof(val));
    }
    inline int64_t get_creation_date(const uint8_t* buf) {
        int64_t val;
        std::memcpy(&val, buf + OFFSET_CREATION_DATE, sizeof(val));
        return val;
    }

    inline void set_birthday(uint8_t* buf, int64_t val) {
        std::memcpy(buf + OFFSET_BIRTHDAY, &val, sizeof(val));
    }
    inline int64_t get_birthday(const uint8_t* buf) {
        int64_t val;
        std::memcpy(&val, buf + OFFSET_BIRTHDAY, sizeof(val));
        return val;
    }

    inline void set_gender(uint8_t* buf, int8_t val) {
        buf[OFFSET_GENDER] = static_cast<uint8_t>(val);
    }
    inline int8_t get_gender(const uint8_t* buf) {
        return static_cast<int8_t>(buf[OFFSET_GENDER]);
    }
}  // namespace SNBPersonSchema

// --- Post vertex schema ---
// | creationDate (int64) | length (int32) |
// | offset 0             | offset 8       |
// Total: 12 bytes

namespace SNBPostSchema {
    constexpr size_t OFFSET_CREATION_DATE = 0;
    constexpr size_t OFFSET_LENGTH        = 8;
    constexpr size_t TOTAL_SIZE           = 12;

    inline void set_creation_date(uint8_t* buf, int64_t val) {
        std::memcpy(buf + OFFSET_CREATION_DATE, &val, sizeof(val));
    }
    inline int64_t get_creation_date(const uint8_t* buf) {
        int64_t val;
        std::memcpy(&val, buf + OFFSET_CREATION_DATE, sizeof(val));
        return val;
    }

    inline void set_length(uint8_t* buf, int32_t val) {
        std::memcpy(buf + OFFSET_LENGTH, &val, sizeof(val));
    }
    inline int32_t get_length(const uint8_t* buf) {
        int32_t val;
        std::memcpy(&val, buf + OFFSET_LENGTH, sizeof(val));
        return val;
    }
}  // namespace SNBPostSchema

// --- knows edge schema ---
// | creationDate (int64) |
// | offset 0             |
// Total: 8 bytes

namespace SNBKnowsSchema {
    constexpr size_t OFFSET_CREATION_DATE = 0;
    constexpr size_t TOTAL_SIZE           = 8;

    inline void set_creation_date(uint8_t* buf, int64_t val) {
        std::memcpy(buf + OFFSET_CREATION_DATE, &val, sizeof(val));
    }
    inline int64_t get_creation_date(const uint8_t* buf) {
        int64_t val;
        std::memcpy(&val, buf + OFFSET_CREATION_DATE, sizeof(val));
        return val;
    }
}  // namespace SNBKnowsSchema

// --- likes edge schema ---
// | creationDate (int64) |
// | offset 0             |
// Total: 8 bytes  (identical layout to SNBKnowsSchema)

namespace SNBLikesSchema {
    constexpr size_t OFFSET_CREATION_DATE = 0;
    constexpr size_t TOTAL_SIZE           = 8;

    inline void set_creation_date(uint8_t* buf, int64_t val) {
        std::memcpy(buf + OFFSET_CREATION_DATE, &val, sizeof(val));
    }
    inline int64_t get_creation_date(const uint8_t* buf) {
        int64_t val;
        std::memcpy(&val, buf + OFFSET_CREATION_DATE, sizeof(val));
        return val;
    }
}  // namespace SNBLikesSchema

// --- hasCreator edge schema ---
// No properties. Zero-size blob.

namespace SNBHasCreatorSchema {
    constexpr size_t TOTAL_SIZE = 0;
}  // namespace SNBHasCreatorSchema

#endif
