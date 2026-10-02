/**
 * @file RecSpec.hpp
 * @brief Runtime-editable binary record layouts for QMD face records.
 *
 * A spec is a Python-struct-like field list, e.g.:
 *   "v0:H v1:H u0:B w0:B cba:H rgb:I n0:H"
 * field = "<role>:<type>"; types: B=u8 b=s8 H=u16le h=s16le I=u32le i=s32le
 *   with '>' suffix for big-endian (e.g. "H>").
 * Roles:
 *   v0..v3  vertex indices (stored x2 -> decoded >>1)
 *   u0..u3 / w0..w3  texture u / v per corner
 *   cba, tp           CLUT / texture-page GPU words
 *   rgb               flat color word (low 3 bytes)
 *   c0..c3            per-corner color (low 3 bytes of the field)
 *   n0..n3            normal data (s8 triplets -> use nXa/nXb/nXc roles for the
 *                     components, or index into the part normal pool when the
 *                     field is u16)
 *   x                 skip/padding
 * Unknown roles are kept as raw values for the inspector.
 */
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sb {

struct RecField {
    std::string role;   // "v0", "u0", "cba", ...
    char type = 'B';    // B b H h I i
    bool bigEndian = false;
    int size = 1;
    size_t offset = 0;  // within the record
};

struct RecSpec {
    std::vector<RecField> fields;
    int size = 0;          // computed record size
    std::string error;     // parse error if any
    bool valid = false;
};

/// Parses a spec string; on error returns invalid spec with .error set.
RecSpec parseRecSpec(const std::string& text);

/// Decoded record: role -> raw value (multi-byte LE/BE honored).
struct RecValues {
    std::map<std::string, uint32_t> v;   // raw values by role
    int32_t sval(const RecField& f, const uint8_t* rec) const;
};

RecValues decodeRecord(const RecSpec& spec, const uint8_t* rec);

/// Default specs (the verified layouts from docs/qmd-format.md).
const std::map<int, std::string>& defaultRecSpecs();

} // namespace sb
