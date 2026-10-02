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
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace sb {

struct QmdVertex { float x, y, z; };

struct QmdFace {
    std::vector<uint32_t> verts;     // 3 or 4 vertex indices
    size_t recOff = 0;               // file offset of the source record
    int recType = -1;                // record type
    uint8_t r = 200, g = 170, b = 90; // flat / first corner color
    bool textured = false;
    uint8_t uv[4][2] = {};           // per-corner texel coords
    uint16_t clut = 0, tpage = 0;    // GPU words
    float n[4][3] = {};              // stored per-corner normals (s8, normalized)
    bool hasNormals = false;
    uint8_t vc[4][3] = {};           // per-corner gouraud colors (types 0/1/4/5)
    bool hasVertColors = false;
};

struct QmdBlock {
    std::string name;
    size_t offset = 0;
    bool simple = true;
    uint32_t vertCount = 0;
    std::vector<QmdVertex> verts;
    std::vector<QmdFace> faces;      // 3- or 4-vert faces; quads in cyclic
                                     // boundary order (v0,v1,v3,v2 of GPU order)
    std::vector<QmdBlock> parts;     // complex blocks: one entry per part
    // file-relative byte ranges of structural sections, for hex highlighting
    std::vector<std::tuple<size_t, size_t, std::string>> regions;
};

/// Runtime parse options (the model lab edits these live; env vars SB_AXES /
/// SB_QPERM / SB_TRIPERM / SB_UVFLIP / SB_PART / SB_NONRM set the initial
/// values for headless runs).
struct ParseOpts {
    int axisSrc[3] = {-1, -1, -1};  // -1 = default (simple: a,c,b; complex: a,b,c)
    int axisSgn[3] = {1, 1, 1};
    int triPerm = 5;                // index into kTriPerms (default 2,1,0 = reversed
                                    // winding -> outward geometric normals)
    int quadPerm = 23;              // index into kQuadPerms (default 3,2,1,0, same)
    int uvFlip = 0;                 // bit0 swap u/v, bit1 mirror u, bit2 mirror v
    int onlyPart = -1;              // complex: render only this part
    bool storedNormals = true;
    int idxShift = 1;               // vertex/normal index downshift (stored x2)
    std::map<int, std::string> recSpecs;   // custom record layouts by type
    std::map<int, int> recSizeOverride;    // record size overrides by type
};
extern ParseOpts g_parseOpts;

/// Walks every QMD block in a part3 buffer and parses simple blocks into
/// geometry. Complex blocks are listed with simple=false.
std::vector<QmdBlock> parseQmdContainer(const uint8_t* data, size_t size);

} // namespace sb
