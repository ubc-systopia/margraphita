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
// | creationDate (int64)  | birthday (int64)    | gender (int8)     |
// | offset 0              | offset 8            | offset 16         |
// | firstName (char[32])  | lastName (char[32]) |
// | offset 17             | offset 49           |
// | browserUsed (char[32])| locationIP (char[32])|
// | offset 81             | offset 113           |
// Total: 8+8+1+32+32+32+32 = 145 bytes
//
// String fields are fixed 32-byte NUL-padded arrays (matching WT format "32s").
// Strings longer than 31 chars are silently truncated.

namespace SNBPersonSchema {
    constexpr size_t STR_LEN               = 32;
    constexpr size_t OFFSET_CREATION_DATE  = 0;
    constexpr size_t OFFSET_BIRTHDAY       = 8;
    constexpr size_t OFFSET_GENDER         = 16;
    constexpr size_t OFFSET_FIRST_NAME     = 17;
    constexpr size_t OFFSET_LAST_NAME      = 17 + STR_LEN;
    constexpr size_t OFFSET_BROWSER_USED   = 17 + STR_LEN * 2;
    constexpr size_t OFFSET_LOCATION_IP    = 17 + STR_LEN * 3;
    constexpr size_t TOTAL_SIZE            = 17 + STR_LEN * 4;  // 145 bytes

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

    // str must be a NUL-terminated C string; copied into a fixed 32-byte field.
    inline void set_str_field(uint8_t* buf, size_t offset, const char* str) {
        char tmp[STR_LEN] = {};
        if (str) std::strncpy(tmp, str, STR_LEN - 1);
        std::memcpy(buf + offset, tmp, STR_LEN);
    }
    // Returns a pointer to the 32-byte field inside buf (NUL-padded, not
    // guaranteed NUL-terminated at byte 31 unless written via set_str_field).
    inline const char* get_str_field(const uint8_t* buf, size_t offset) {
        return reinterpret_cast<const char*>(buf + offset);
    }

    inline void set_first_name(uint8_t* buf, const char* s)    { set_str_field(buf, OFFSET_FIRST_NAME,   s); }
    inline void set_last_name(uint8_t* buf, const char* s)     { set_str_field(buf, OFFSET_LAST_NAME,    s); }
    inline void set_browser_used(uint8_t* buf, const char* s)  { set_str_field(buf, OFFSET_BROWSER_USED, s); }
    inline void set_location_ip(uint8_t* buf, const char* s)   { set_str_field(buf, OFFSET_LOCATION_IP,  s); }

    inline const char* get_first_name(const uint8_t* buf)   { return get_str_field(buf, OFFSET_FIRST_NAME);   }
    inline const char* get_last_name(const uint8_t* buf)    { return get_str_field(buf, OFFSET_LAST_NAME);    }
    inline const char* get_browser_used(const uint8_t* buf) { return get_str_field(buf, OFFSET_BROWSER_USED); }
    inline const char* get_location_ip(const uint8_t* buf)  { return get_str_field(buf, OFFSET_LOCATION_IP);  }
}  // namespace SNBPersonSchema

// --- Post vertex schema ---
// | creationDate (int64) | length (int32) | tag (uint8) | content (NUL-terminated) |
// | offset 0             | offset 8       | offset 12   | offset 13                |
// TOTAL_SIZE = 13 (minimum, with empty content).  Actual blob = 13 + strlen(content) + 1.
// tag: 0 = text content, 1 = imageFile name.
// content field: at most CONTENT_MAX_LEN chars (truncated on write).
// In WT COLUMNAR: value_format=QibS  (colgroups: temporal=(creationDate,length),
//                                                content=(tag,content))

namespace SNBPostSchema {
    constexpr size_t CONTENT_MAX_LEN      = 2000;
    constexpr size_t OFFSET_CREATION_DATE = 0;
    constexpr size_t OFFSET_LENGTH        = 8;
    constexpr size_t OFFSET_TAG           = 12;
    constexpr size_t OFFSET_CONTENT       = 13;
    constexpr size_t TOTAL_SIZE           = 13;  // minimum blob size

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

    inline void set_tag(uint8_t* buf, int8_t tag) {
        buf[OFFSET_TAG] = static_cast<uint8_t>(tag);
    }
    inline int8_t get_tag(const uint8_t* buf) {
        return static_cast<int8_t>(buf[OFFSET_TAG]);
    }

    // content must be NUL-terminated and at most CONTENT_MAX_LEN chars.
    // buf must be large enough: TOTAL_SIZE + strlen(content) + 1.
    inline void set_content(uint8_t* buf, const char* content) {
        if (content)
            std::strcpy(reinterpret_cast<char*>(buf + OFFSET_CONTENT), content);
        else
            buf[OFFSET_CONTENT] = 0;
    }
    inline const char* get_content(const uint8_t* buf, size_t blob_size) {
        if (blob_size <= OFFSET_CONTENT) return "";
        return reinterpret_cast<const char*>(buf + OFFSET_CONTENT);
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

// ============================================================
// Full LDBC SNB schema additions
// ============================================================

// --- Comment vertex schema ---
// Identical layout to SNBPostSchema (same Message interface).
// tag: 0 always (Comments have no imageFile), kept for schema uniformity.
// In WT COLUMNAR: value_format=QibS
//   colgroups: temporal=(creationDate,length)  content=(tag,content)

namespace SNBCommentSchema {
    using namespace SNBPostSchema;  // reuse all offsets and accessors
}  // namespace SNBCommentSchema

// --- Forum vertex schema ---
// | creationDate(int64) | moderator_id(int64) | title(NUL-terminated string) |
// | offset 0            | offset 8            | offset 16                    |
// TOTAL_SIZE = 16 minimum (variable: +strlen(title)+1).
// moderator_id: compact Flexograph node_id_t of the moderator Person.
//               Set to 0 if no moderator (spec allows 0..1).
// In WT COLUMNAR: value_format=QQS
//   colgroups: temporal=(creationDate)  info=(moderator_id,title)

namespace SNBForumSchema {
    constexpr size_t TITLE_MAX_LEN         = 256;
    constexpr size_t OFFSET_CREATION_DATE  = 0;
    constexpr size_t OFFSET_MODERATOR_ID   = 8;
    constexpr size_t OFFSET_TITLE          = 16;
    constexpr size_t TOTAL_SIZE            = 16;  // minimum blob size

    inline void set_creation_date(uint8_t *buf, int64_t val) {
        std::memcpy(buf + OFFSET_CREATION_DATE, &val, sizeof(val));
    }
    inline int64_t get_creation_date(const uint8_t *buf) {
        int64_t val; std::memcpy(&val, buf + OFFSET_CREATION_DATE, sizeof(val)); return val;
    }
    inline void set_moderator_id(uint8_t *buf, int64_t val) {
        std::memcpy(buf + OFFSET_MODERATOR_ID, &val, sizeof(val));
    }
    inline int64_t get_moderator_id(const uint8_t *buf) {
        int64_t val; std::memcpy(&val, buf + OFFSET_MODERATOR_ID, sizeof(val)); return val;
    }
    // title must be NUL-terminated and at most TITLE_MAX_LEN chars.
    // buf must be large enough: TOTAL_SIZE + strlen(title) + 1.
    inline void set_title(uint8_t *buf, const char *title) {
        if (title) std::strcpy(reinterpret_cast<char *>(buf + OFFSET_TITLE), title);
        else        buf[OFFSET_TITLE] = 0;
    }
    inline const char *get_title(const uint8_t *buf, size_t blob_size) {
        if (blob_size <= OFFSET_TITLE) return "";
        return reinterpret_cast<const char *>(buf + OFFSET_TITLE);
    }
}  // namespace SNBForumSchema

// --- Tag vertex schema ---
// | name(char[80]) | url(char[80]) |
// | offset 0       | offset 80     |
// Total: 160 bytes (fixed).
// In WT COLUMNAR: value_format=80s80s  columns=(node_id,name,url)
// No colgroup — dimension type, used only for point lookups.

namespace SNBTagSchema {
    constexpr size_t STR_LEN    = 80;
    constexpr size_t OFFSET_NAME = 0;
    constexpr size_t OFFSET_URL  = STR_LEN;
    constexpr size_t TOTAL_SIZE  = STR_LEN * 2;  // 160 bytes

    inline void set_str_field(uint8_t *buf, size_t offset, const char *str) {
        char tmp[STR_LEN] = {};
        if (str) std::strncpy(tmp, str, STR_LEN - 1);
        std::memcpy(buf + offset, tmp, STR_LEN);
    }
    inline const char *get_str_field(const uint8_t *buf, size_t offset) {
        return reinterpret_cast<const char *>(buf + offset);
    }
    inline void set_name(uint8_t *buf, const char *s) { set_str_field(buf, OFFSET_NAME, s); }
    inline void set_url (uint8_t *buf, const char *s) { set_str_field(buf, OFFSET_URL,  s); }
    inline const char *get_name(const uint8_t *buf) { return get_str_field(buf, OFFSET_NAME); }
    inline const char *get_url (const uint8_t *buf) { return get_str_field(buf, OFFSET_URL);  }
}  // namespace SNBTagSchema

// --- TagClass vertex schema ---
// Identical layout to SNBTagSchema (name + url, 80 bytes each).

namespace SNBTagClassSchema {
    using namespace SNBTagSchema;
}  // namespace SNBTagClassSchema

// --- Place vertex schema ---
// | name(char[80]) | url(char[80]) | place_type(int8) |
// | offset 0       | offset 80     | offset 160       |
// Total: 161 bytes.
// place_type: 0=city, 1=country, 2=continent.
// In WT COLUMNAR: value_format=80s80sb  columns=(node_id,name,url,place_type)

namespace SNBPlaceSchema {
    constexpr size_t STR_LEN         = 80;
    constexpr size_t OFFSET_NAME     = 0;
    constexpr size_t OFFSET_URL      = STR_LEN;
    constexpr size_t OFFSET_TYPE     = STR_LEN * 2;
    constexpr size_t TOTAL_SIZE      = STR_LEN * 2 + 1;  // 161 bytes

    constexpr int8_t TYPE_CITY      = 0;
    constexpr int8_t TYPE_COUNTRY   = 1;
    constexpr int8_t TYPE_CONTINENT = 2;

    inline void set_str_field(uint8_t *buf, size_t offset, const char *str) {
        char tmp[STR_LEN] = {};
        if (str) std::strncpy(tmp, str, STR_LEN - 1);
        std::memcpy(buf + offset, tmp, STR_LEN);
    }
    inline const char *get_str_field(const uint8_t *buf, size_t offset) {
        return reinterpret_cast<const char *>(buf + offset);
    }
    inline void set_name(uint8_t *buf, const char *s) { set_str_field(buf, OFFSET_NAME, s); }
    inline void set_url (uint8_t *buf, const char *s) { set_str_field(buf, OFFSET_URL,  s); }
    inline void set_place_type(uint8_t *buf, int8_t t) { buf[OFFSET_TYPE] = static_cast<uint8_t>(t); }
    inline const char *get_name(const uint8_t *buf)  { return get_str_field(buf, OFFSET_NAME); }
    inline const char *get_url (const uint8_t *buf)  { return get_str_field(buf, OFFSET_URL);  }
    inline int8_t get_place_type(const uint8_t *buf) { return static_cast<int8_t>(buf[OFFSET_TYPE]); }
}  // namespace SNBPlaceSchema

// --- Organisation vertex schema ---
// | name(char[80]) | url(char[80]) | org_type(int8) |
// | offset 0       | offset 80     | offset 160     |
// Total: 161 bytes.
// org_type: 0=company, 1=university.
// In WT COLUMNAR: value_format=80s80sb  columns=(node_id,name,url,org_type)

namespace SNBOrganisationSchema {
    constexpr size_t STR_LEN         = 80;
    constexpr size_t OFFSET_NAME     = 0;
    constexpr size_t OFFSET_URL      = STR_LEN;
    constexpr size_t OFFSET_TYPE     = STR_LEN * 2;
    constexpr size_t TOTAL_SIZE      = STR_LEN * 2 + 1;  // 161 bytes

    constexpr int8_t TYPE_COMPANY    = 0;
    constexpr int8_t TYPE_UNIVERSITY = 1;

    inline void set_str_field(uint8_t *buf, size_t offset, const char *str) {
        char tmp[STR_LEN] = {};
        if (str) std::strncpy(tmp, str, STR_LEN - 1);
        std::memcpy(buf + offset, tmp, STR_LEN);
    }
    inline const char *get_str_field(const uint8_t *buf, size_t offset) {
        return reinterpret_cast<const char *>(buf + offset);
    }
    inline void set_name(uint8_t *buf, const char *s) { set_str_field(buf, OFFSET_NAME, s); }
    inline void set_url (uint8_t *buf, const char *s) { set_str_field(buf, OFFSET_URL,  s); }
    inline void set_org_type(uint8_t *buf, int8_t t) { buf[OFFSET_TYPE] = static_cast<uint8_t>(t); }
    inline const char *get_name(const uint8_t *buf)  { return get_str_field(buf, OFFSET_NAME); }
    inline const char *get_url (const uint8_t *buf)  { return get_str_field(buf, OFFSET_URL);  }
    inline int8_t get_org_type(const uint8_t *buf)   { return static_cast<int8_t>(buf[OFFSET_TYPE]); }
}  // namespace SNBOrganisationSchema

// --- hasMember edge schema ---
// | creationDate(int64) |
// | offset 0            |
// Total: 8 bytes.
// In WT COLUMNAR: value_format=Q  colgroup: temporal=(creationDate)

namespace SNBHasMemberSchema {
    constexpr size_t OFFSET_CREATION_DATE = 0;
    constexpr size_t TOTAL_SIZE           = 8;

    inline void set_creation_date(uint8_t *buf, int64_t val) {
        std::memcpy(buf + OFFSET_CREATION_DATE, &val, sizeof(val));
    }
    inline int64_t get_creation_date(const uint8_t *buf) {
        int64_t val; std::memcpy(&val, buf + OFFSET_CREATION_DATE, sizeof(val)); return val;
    }
}  // namespace SNBHasMemberSchema

// --- studyAt edge schema ---
// | classYear(int32) |
// | offset 0         |
// Total: 4 bytes.
// In WT COLUMNAR: value_format=i

namespace SNBStudyAtSchema {
    constexpr size_t OFFSET_CLASS_YEAR = 0;
    constexpr size_t TOTAL_SIZE        = 4;

    inline void set_class_year(uint8_t *buf, int32_t val) {
        std::memcpy(buf + OFFSET_CLASS_YEAR, &val, sizeof(val));
    }
    inline int32_t get_class_year(const uint8_t *buf) {
        int32_t val; std::memcpy(&val, buf + OFFSET_CLASS_YEAR, sizeof(val)); return val;
    }
}  // namespace SNBStudyAtSchema

// --- workAt edge schema ---
// | workFrom(int32) |
// | offset 0        |
// Total: 4 bytes.
// In WT COLUMNAR: value_format=i

namespace SNBWorkAtSchema {
    constexpr size_t OFFSET_WORK_FROM = 0;
    constexpr size_t TOTAL_SIZE       = 4;

    inline void set_work_from(uint8_t *buf, int32_t val) {
        std::memcpy(buf + OFFSET_WORK_FROM, &val, sizeof(val));
    }
    inline int32_t get_work_from(const uint8_t *buf) {
        int32_t val; std::memcpy(&val, buf + OFFSET_WORK_FROM, sizeof(val)); return val;
    }
}  // namespace SNBWorkAtSchema

#endif
