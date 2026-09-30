#include "QmdModel.hpp"

#include <cstring>

namespace sb {

static uint16_t rd16(const uint8_t* p) { return p[0] | (p[1] << 8); }
static int16_t rds16(const uint8_t* p) { return (int16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

// Face chunk record sizes by type (docs/qmd-format.md):
// 0 quad gouraud 36 | 1 tri gouraud 28 | 2 quad flat 24 | 3 tri flat 20
// 4 quad gouraud nonorm 24 | 5 tri gouraud nonorm 24 | 6 two-quad strip 28
// 8 quad GT4 textured 42 | 10 quad FT4 textured 36
static int recSize(int type) {
    switch (type) {
        case 0: return 36;
        case 1: return 28;
        case 2: case 4: case 5: return 24;
        case 3: return 20;
        case 6: return 28;
        case 8: return 42;
        case 10: return 36;
        default: return 0;
    }
}

// Vertex index read: stored x2.
static inline uint32_t vidx(const uint8_t* r, int j) { return rd16(r + 2 * j) >> 1; }

// First-corner / per-corner color offset per type (colors are 4B RGBA quads).
static int colorOff(int type, int corner, bool& ok) {
    ok = true;
    switch (type) {
        case 0: case 8: return 20 + 4 * corner;   // 4 colors
        case 1: return 16 + 4 * corner;           // 3 colors
        case 4: return 8 + 4 * corner;            // 4 colors, no normals
        case 5: return 6 + 4 * corner;            // 3 colors, no normals
        case 2: case 10: return 20;               // 1 color
        case 3: return 16;                        // 1 color
        case 6: return 24;                        // 1 color
        default: ok = false; return 0;
    }
}

static void parseSimpleBlock(const uint8_t* data, size_t size, size_t boff, QmdBlock& out) {
    const uint8_t* base = data + boff + 0x10;
    size_t baseOff = boff + 0x10;
    uint32_t f0 = rd32(base + 0x00);  // section A (face chunks)
    uint32_t f1 = rd32(base + 0x04);  // section B (x,z pairs)
    uint32_t f2 = rd32(base + 0x08);  // section C (y)
    out.vertCount = rd32(base + 0x14);

    // vertices: B = n x (s16 x, s16 z), C = n x s16 y
    size_t vb = baseOff + f1, vc = baseOff + f2;
    if (vb + out.vertCount * 4 > size || vc + out.vertCount * 2 > size) return;
    out.verts.reserve(out.vertCount);
    for (uint32_t i = 0; i < out.vertCount; ++i) {
        QmdVertex v;
        v.x = rds16(data + vb + 4 * i);
        v.y = rds16(data + vc + 2 * i);
        v.z = rds16(data + vb + 4 * i + 2);
        out.verts.push_back(v);
    }

    // face chunks
    size_t p = baseOff + f0, end = baseOff + f1;
    while (p + 4 <= end && p + 4 <= size) {
        uint16_t count = rd16(data + p), type = rd16(data + p + 2);
        if (count == 0 && type == 0) break;
        int rs = recSize(type);
        if (rs == 0 || p + 4 + (size_t)rs * count > size) break;
        p += 4;
        for (uint16_t i = 0; i < count; ++i) {
            const uint8_t* r = data + p + (size_t)rs * i;
            QmdFace face;
            int n = (type == 1 || type == 3 || type == 5) ? 3 : 4;
            for (int j = 0; j < n; ++j) {
                uint32_t vi = vidx(r, j);
                if (vi >= out.vertCount) { face.verts.clear(); break; }
                face.verts.push_back(vi);
            }
            if (face.verts.empty()) continue;
            face.textured = (type == 8 || type == 10);
            bool ok;
            int co = colorOff(type, 0, ok);
            if (ok) {
                face.r = r[co]; face.g = r[co + 1]; face.b = r[co + 2];
            }
            // triangulate quads as (v0,v1,v2)+(v1,v2,v3)
            if (face.verts.size() == 4) {
                QmdFace q = face;
                face.verts = {q.verts[0], q.verts[1], q.verts[2]};
                QmdFace t2 = q;
                t2.verts = {q.verts[1], q.verts[2], q.verts[3]};
                out.faces.push_back(std::move(face));
                out.faces.push_back(std::move(t2));
            } else {
                out.faces.push_back(std::move(face));
            }
        }
        p += (size_t)rs * count;
    }
}

std::vector<QmdBlock> parseQmdContainer(const uint8_t* data, size_t size) {
    std::vector<QmdBlock> out;
    for (size_t i = 0; i + 12 <= size; ++i) {
        if (std::memcmp(data + i, "QMD ", 4) != 0) continue;
        // name must be printable ascii
        bool ok = true;
        for (int j = 0; j < 8; ++j) {
            uint8_t c = data[i + 4 + j];
            if (c < 0x20 || c > 0x7E) { ok = false; break; }
        }
        if (!ok) continue;
        QmdBlock blk;
        blk.offset = i;
        blk.name.assign((const char*)data + i + 4, 8);
        uint32_t kind = rd32(data + i + 0x0C);
        blk.simple = (kind & 0xFFFF) == 1;
        if (blk.simple) parseSimpleBlock(data, size, i, blk);
        out.push_back(std::move(blk));
    }
    return out;
}

} // namespace sb
