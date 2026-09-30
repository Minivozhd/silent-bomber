/**
 * @file QmdModel.hpp
 * @brief Silent Bomber QMD model/scene parsing (port of experiments/qmd_parse.py,
 *        see docs/qmd-format.md).
 *
 * part3 container: "pQES" magic + header, then a chain of QMD blocks.
 * QMD block: "QMD " + 8-byte name + kind word (lo u16: 1 = simple, N = complex
 * part count — complex parsing not ported yet) + header fields.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sb {

struct QmdVertex { float x, y, z; };

struct QmdFace {
    std::vector<uint32_t> verts;     // 3 or 4 vertex indices
    uint8_t r = 200, g = 170, b = 90; // flat / first corner color
    bool textured = false;
};

struct QmdBlock {
    std::string name;
    size_t offset = 0;
    bool simple = true;
    uint32_t vertCount = 0;
    std::vector<QmdVertex> verts;
    std::vector<QmdFace> faces;      // triangulated on parse
};

/// Walks every QMD block in a part3 buffer and parses simple blocks into
/// geometry. Complex blocks are listed with simple=false.
std::vector<QmdBlock> parseQmdContainer(const uint8_t* data, size_t size);

} // namespace sb
