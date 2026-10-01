#include "QmdModel.hpp"

#include <cstdlib>
#include <cstring>

namespace sb {

static uint16_t rd16(const uint8_t* p) { return p[0] | (p[1] << 8); }
static int16_t rds16(const uint8_t* p) { return (int16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

// Face chunk record sizes by type (docs/qmd-format.md):
// 0 quad gouraud 36 | 1 tri gouraud 28 | 2 quad flat 24 | 3 tri flat 20
// 4 quad gouraud nonorm 24 | 5 tri gouraud nonorm 24 | 6 two-quad strip 28
// 8 quad GT4 textured 32 | 9 tri GT3 textured 28
// 10 quad FT4 textured 32 | 11 tri FT3 textured 28
static int recSize(int type) {
    switch (type) {
        case 0: return 36;
        case 1: return 28;
        case 2: case 4: case 5: return 24;
        case 3: return 20;
        case 6: return 28;
        case 8: case 10: return 32;
        case 9: case 11: return 28;
        default: return 0;
    }
}

// Vertex index read: stored x2.
static inline uint32_t vidx(const uint8_t* r, int j) { return rd16(r + 2 * j) >> 1; }

// First-corner / per-corner color offset per type (colors are 4B RGBA quads).
static int colorOff(int type, int corner, bool& ok) {
    ok = true;
    switch (type) {
        case 0: return 20 + 4 * corner;           // 4 colors
        case 1: return 16 + 4 * corner;           // 3 colors
        case 4: return 8 + 4 * corner;            // 4 colors, no normals
        case 5: return 6 + 4 * corner;            // 3 colors, no normals
        case 2: case 10: return 20;               // 1 color (trailer rgb+cmd)
        case 3: case 9: case 11: return 16;       // 1 color (trailer rgb+cmd)
        case 6: return 24;                        // 1 color
        case 8: return 20;                        // 1 color (trailer rgb+cmd)
        default: ok = false; return 0;
    }
}

static void parseSimpleBlock(const uint8_t* data, size_t size, size_t boff, QmdBlock& out);

// Shared face-chunk stream parser (same record layout in simple and complex
// blocks): u16 count + u16 type chunks until a (0,0) terminator.
// Vertex indices are stored x2 and are local to the given vertex window
// [vbase, vbase + vcount) in out.verts.
// Quad records store corners in GPU order: the GPU draws (v0,v1,v2)+(v1,v2,v3),
// i.e. the polygon boundary is the zigzag v0-v1-v3-v2 (verified: this is the
// only convex boundary, and the 3D<->UV edge-ratio variance is minimal for the
// identity mapping). SB_QPERM allows A/B testing of alternate interpretations.
static const int kQuadPerms[][4] = {
    {0, 1, 2, 3}, {3, 2, 0, 1}, {2, 0, 1, 3}, {3, 1, 0, 2}, {2, 3, 1, 0}, {1, 3, 2, 0},
};
static const int* quadPerm() {
    static int p = [] {
        const char* s = getenv("SB_QPERM");
        int v = s ? atoi(s) : 0;
        return (v >= 0 && v < 6) ? v : 0;
    }();
    return kQuadPerms[p];
}

// Axis mapping: complex model blocks store (x, y) pairs + z pool
// (verified via stored normals on EMBTNK00); simple blocks (levels, props)
// store (x, z) pairs + y pool. SB_SWAPYZ forces the swap for A/B testing:
// 0 = auto (complex: y from pair; simple: y from pool), 1 = force pool-y,
// 2 = force pair-y.
static int swapYzMode() {
    static int m = [] { const char* s = getenv("SB_SWAPYZ"); return s ? atoi(s) : 0; }();
    return m;
}

static void parseChunkStream(const uint8_t* data, size_t size, size_t p,
                             uint32_t vbase, uint32_t vcount, QmdBlock& out,
                             const uint8_t* nrmPool = nullptr,
                             const uint8_t* f4Pool = nullptr,
                             uint32_t nrmSlots = 0) {
    while (p + 4 <= size) {
        uint16_t count = rd16(data + p), type = rd16(data + p + 2);
        if (count == 0 && type == 0) break;
        int rs = recSize(type);
        if (rs == 0 || p + 4 + (size_t)rs * count > size) break;
        p += 4;
        for (uint16_t i = 0; i < count; ++i) {
            const uint8_t* r = data + p + (size_t)rs * i;
            QmdFace face;
            int n = (type == 1 || type == 3 || type == 5 || type == 9 || type == 11) ? 3 : 4;
            uint32_t rawIdx[4];
            for (int j = 0; j < n; ++j) rawIdx[j] = vidx(r, j);
            const int* pm = quadPerm();
            for (int j = 0; j < n; ++j) {
                uint32_t vi = rawIdx[n == 4 ? pm[j] : j];
                if (vi >= vcount) { face.verts.clear(); break; }
                face.verts.push_back(vbase + vi);
            }
            if (face.verts.empty()) continue;
            face.textured = (type >= 8);
            bool isQuad = (n == 4);
            // stored per-corner normals for the untextured types (s8 x3, normalized)
            if (type == 0 || type == 2) {          // [8B idx][12B: 4x3 normals][...]
                face.hasNormals = true;
                for (int j = 0; j < 4; ++j) {
                    int tj = isQuad ? pm[j] : j;
                    int8_t nx = (int8_t)r[8 + 3 * tj], ny = (int8_t)r[8 + 3 * tj + 1], nz = (int8_t)r[8 + 3 * tj + 2];
                    float l = std::sqrt((float)(nx * nx + ny * ny + nz * nz));
                    if (l > 1e-6f) { face.n[j][0] = nx / l; face.n[j][1] = ny / l; face.n[j][2] = nz / l; }
                }
            } else if (type == 1 || type == 3) {   // [idx][9B: 3x3 normals(+pad)]
                face.hasNormals = true;
                for (int j = 0; j < 3; ++j) {
                    int8_t nx = (int8_t)r[6 + 3 * j], ny = (int8_t)r[6 + 3 * j + 1], nz = (int8_t)r[6 + 3 * j + 2];
                    float l = std::sqrt((float)(nx * nx + ny * ny + nz * nz));
                    if (l > 1e-6f) { face.n[j][0] = nx / l; face.n[j][1] = ny / l; face.n[j][2] = nz / l; }
                }
            }
            // UV block for textured types (uv = u8 pairs, cba/tpage = u16)
            if (type == 8 || type == 10) {       // quad: [8B idx][12B uvblk][4B rgb+cmd][8B nidx]
                uint8_t rawUv[4][2];
                for (int j = 0; j < 3; ++j) {
                    rawUv[j][0] = r[8 + 2 * j];
                    rawUv[j][1] = r[8 + 2 * j + 1];
                }
                face.clut = rd16(r + 14);
                rawUv[3][0] = r[16]; rawUv[3][1] = r[17];
                face.tpage = rd16(r + 18);
                for (int j = 0; j < 4; ++j) {
                    face.uv[j][0] = rawUv[pm[j]][0];
                    face.uv[j][1] = rawUv[pm[j]][1];
                }
            } else if (type == 9 || type == 11) { // tri: [6B idx][10B uvblk][4B rgb+cmd][8B]
                face.uv[0][0] = r[6]; face.uv[0][1] = r[7];
                face.uv[1][0] = r[8]; face.uv[1][1] = r[9];
                face.clut = rd16(r + 10);
                face.uv[2][0] = r[12]; face.uv[2][1] = r[13];
                face.tpage = rd16(r + 14);
            }
            // gouraud types 8/9 (complex blocks): 8B tail = per-corner normal
            // indices x2 into the part's normal pool; normal = (nrm.s16lo,
            // nrm.s16hi, f4.s16) / 4096 [VERIFIED: avg |dot| with adjacent-face
            // geometric normals = 0.997 on EMBTNK00 part4]
            if ((type == 8 || type == 9) && nrmPool && f4Pool) {
                int nn = (type == 8) ? 4 : 3;
                face.hasNormals = true;
                for (int j = 0; j < nn; ++j) {
                    uint32_t s = vidx(r + rs - 8, j);
                    if (s >= nrmSlots) { face.hasNormals = false; break; }
                    float nx = rds16(nrmPool + 4 * s);
                    float ny = rds16(nrmPool + 4 * s + 2);
                    float nz = rds16(f4Pool + 2 * s);
                    float l = std::sqrt(nx * nx + ny * ny + nz * nz);
                    if (l > 1e-6f) { face.n[j][0] = nx / l; face.n[j][1] = ny / l; face.n[j][2] = nz / l; }
                }
            }
            bool ok;
            int co = colorOff(type, 0, ok);
            if (ok) {
                face.r = r[co]; face.g = r[co + 1]; face.b = r[co + 2];
            }
            // Quads: records are in GPU packet order (tris (v0,v1,v2)+(v1,v2,v3)),
            // so the polygon boundary is the zigzag v0-v1-v3-v2. Reorder to the
            // cyclic boundary and keep quads as single 4-vert faces — the PS1
            // GPU maps a quad as one affine primitive; triangulating would
            // create a diagonal UV seam.
            if (face.verts.size() == 4) {
                static const int cyc[4] = {0, 1, 3, 2};
                QmdFace q = face;
                for (int j = 0; j < 4; ++j) {
                    face.verts[j] = q.verts[cyc[j]];
                    face.uv[j][0] = q.uv[cyc[j]][0];
                    face.uv[j][1] = q.uv[cyc[j]][1];
                    for (int a = 0; a < 3; ++a) face.n[j][a] = q.n[cyc[j]][a];
                }
            }
            out.faces.push_back(std::move(face));
        }
        p += (size_t)rs * count;
    }
}

// Complex block: N parts, each a 0x24 table entry whose offsets are
// SELF-RELATIVE to the entry's own address (stored + 0x24 * part_index):
//   +0x00 prim  -> per-part face-chunk stream (same layout as simple blocks)
//   +0x04 xy    -> count x (s16 x, s16 y) vertex pairs
//   +0x08 z     -> count x s16 vertex depths
//   +0x0C nrm   -> 4-byte normals: one per PRIM for flat types (10/11),
//                  one per VERTEX for gouraud types (8/9, indexed x2 from
//                  the 8-byte record tail)
//   +0x10 f4    -> u16 array, same per-prim/per-vertex multiplicity as nrm
//   +0x14 count -> vertex count of this part
//   +0x18 f6    -> offset into the prim region (runtime use)
//   +0x1C bbox  -> 4 x s16 bounding sphere (cx, cy, cz, r)
static void parseComplexBlock(const uint8_t* data, size_t size, size_t boff,
                              uint32_t parts, QmdBlock& out) {
    size_t baseOff = boff + 0x10;
    int onlyPart = [] { const char* s = getenv("SB_PART"); return s ? atoi(s) : -1; }();
    for (uint32_t k = 0; k < parts; ++k) {
        size_t eoff = baseOff + 0x24 * k;
        if (eoff + 0x24 > size) return;
        const uint8_t* e = data + eoff;
        uint32_t fix = 0x24 * k;  // self-relative offset fixup
        uint32_t prim = rd32(e) + fix;
        uint32_t xz = rd32(e + 4) + fix;
        uint32_t y = rd32(e + 8) + fix;
        uint32_t nrm = rd32(e + 12) + fix;
        uint32_t f4 = rd32(e + 16) + fix;
        uint32_t count = rd32(e + 20);
        size_t vb = baseOff + xz, vc = baseOff + y;
        if (vb + count * 4 > size || vc + count * 2 > size) return;
        if (baseOff + nrm + count * 4 > size || baseOff + f4 + count * 2 > size) return;

        QmdBlock sub;
        sub.name = out.name + " part " + std::to_string(k);
        sub.offset = boff + eoff - baseOff;
        sub.simple = false;
        sub.vertCount = count;
        bool poolY = swapYzMode() == 1;  // complex default: y from the pair
        for (uint32_t j = 0; j < count; ++j) {
            QmdVertex v;
            v.x = rds16(data + vb + 4 * j);
            int16_t p1 = rds16(data + vb + 4 * j + 2);
            int16_t pc = rds16(data + vc + 2 * j);
            v.y = poolY ? pc : p1;
            v.z = poolY ? p1 : pc;
            sub.verts.push_back(v);
        }
        parseChunkStream(data, size, baseOff + prim, 0, count, sub,
                         data + baseOff + nrm, data + baseOff + f4, count);
        if (onlyPart < 0 || (int)k == onlyPart) {
            // merge into the assembled view
            uint32_t vbase = (uint32_t)out.verts.size();
            out.verts.insert(out.verts.end(), sub.verts.begin(), sub.verts.end());
            for (auto f : sub.faces) {
                for (auto& vi : f.verts) vi += vbase;
                out.faces.push_back(std::move(f));
            }
        }
        out.parts.push_back(std::move(sub));
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
    bool pairY = swapYzMode() == 2;  // simple default: y from the pool
    for (uint32_t i = 0; i < out.vertCount; ++i) {
        QmdVertex v;
        v.x = rds16(data + vb + 4 * i);
        int16_t p1 = rds16(data + vb + 4 * i + 2);
        int16_t pc = rds16(data + vc + 2 * i);
        v.y = pairY ? p1 : pc;
        v.z = pairY ? pc : p1;
        out.verts.push_back(v);
    }

    parseChunkStream(data, size, baseOff + f0, 0, out.vertCount, out);
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
        else parseComplexBlock(data, size, i, kind & 0xFFFF, blk);
        out.push_back(std::move(blk));
    }
    return out;
}

} // namespace sb
