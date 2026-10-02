#include "QmdModel.hpp"

#include "RecSpec.hpp"

#include <algorithm>
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
    {0, 1, 2, 3}, {0, 1, 3, 2}, {0, 2, 1, 3}, {0, 2, 3, 1}, {0, 3, 1, 2}, {0, 3, 2, 1},
    {1, 0, 2, 3}, {1, 0, 3, 2}, {1, 2, 0, 3}, {1, 2, 3, 0}, {1, 3, 0, 2}, {1, 3, 2, 0},
    {2, 0, 1, 3}, {2, 0, 3, 1}, {2, 1, 0, 3}, {2, 1, 3, 0}, {2, 3, 0, 1}, {2, 3, 1, 0},
    {3, 0, 1, 2}, {3, 0, 2, 1}, {3, 1, 0, 2}, {3, 1, 2, 0}, {3, 2, 0, 1}, {3, 2, 1, 0},
};
ParseOpts g_parseOpts;

static bool initParseOpts() {
    if (const char* e = getenv("SB_QPERM")) g_parseOpts.quadPerm = std::clamp(atoi(e), 0, 23);
    if (const char* e = getenv("SB_TRIPERM")) g_parseOpts.triPerm = std::clamp(atoi(e), 0, 5);
    if (const char* e = getenv("SB_UVFLIP")) g_parseOpts.uvFlip = atoi(e);
    if (const char* e = getenv("SB_PART")) g_parseOpts.onlyPart = atoi(e);
    if (getenv("SB_NONRM")) g_parseOpts.storedNormals = false;
    if (getenv("SB_UV2")) g_parseOpts.uv2Mode = atoi(getenv("SB_UV2"));
    if (const char* e = getenv("SB_AXES")) {
        // "a,c,b" with optional '-' sign prefix per component
        int idx = 0;
        for (size_t i = 0; e[i] && idx < 3; ++i) {
            int sg = 1;
            if (e[i] == '-') { sg = -1; ++i; }
            if (e[i] >= 'a' && e[i] <= 'c') {
                g_parseOpts.axisSrc[idx] = e[i] - 'a';
                g_parseOpts.axisSgn[idx] = sg;
                ++idx;
            }
        }
    }
    return true;
}
static const bool kOptsInit = initParseOpts();
static const int* quadPerm() { return kQuadPerms[g_parseOpts.quadPerm]; }

// Axis mapping: complex model blocks store (x, y) pairs + z pool
// (verified via stored normals on EMBTNK00); simple blocks (levels, props)
// store (x, z) pairs + y pool. SB_SWAPYZ forces the swap for A/B testing:
// 0 = auto (complex: y from pair; simple: y from pool), 1 = force pool-y,
// 2 = force pair-y.
// Axis source mapping: SB_AXES="a,c,b" picks the source for x,y,z:
//   a = pair first half, b = pair second half, c = pool; prefix '-' negates.
// Defaults: simple blocks "a,c,b" (levels), complex blocks "a,b,c".
struct Axes { int src[3]; int sgn[3]; };
static Axes axesEnv(const char* def) {
    Axes ax;
    for (int k = 0; k < 3; ++k) {
        ax.src[k] = (g_parseOpts.axisSrc[k] >= 0) ? g_parseOpts.axisSrc[k] : def[k * 2] - 'a';
        ax.sgn[k] = g_parseOpts.axisSgn[k];
    }
    return ax;
}
// SB_TRIPERM: vertex/uv corner permutation for tri records (0..5).
// SB_UVFLIP: bit0 swap u/v, bit1 mirror u (255-u), bit2 mirror v (255-v).
static const int kTriPerms[][3] = {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
static int triPermMode() { return g_parseOpts.triPerm; }
// pool-normal component remap presets (before axis transform)
static const int kNrmPerms[][3] = {
    {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0},
};
static void mapNormal(const float raw[3], float out[3]) {
    const int* mp = kNrmPerms[g_parseOpts.nrmPerm];
    for (int k = 0; k < 3; ++k) out[k] = raw[mp[k]] * g_parseOpts.nrmSgn[k];
}
static int uvFlipMode() { return g_parseOpts.uvFlip; }

static void parseChunkStream(const uint8_t* data, size_t size, size_t p,
                             uint32_t vbase, uint32_t vcount, QmdBlock& out,
                             const uint8_t* nrmPool = nullptr,
                             const uint8_t* f4Pool = nullptr,
                             uint32_t nrmSlots = 0,
                             bool s8normals = false,
                             const Axes* axes = nullptr) {
    const int* pm = quadPerm();
    const int* tm = kTriPerms[triPermMode()];
    while (p + 4 <= size) {
        uint16_t count = rd16(data + p), type = rd16(data + p + 2);
        if (count == 0 && type == 0) break;
        // record layout: custom spec > defaults; size: override > spec > table
        RecSpec spec;
        bool custom = false;
        auto sit = g_parseOpts.recSpecs.find(type);
        if (sit != g_parseOpts.recSpecs.end()) {
            spec = parseRecSpec(sit->second);
            custom = true;
        } else {
            auto dit = defaultRecSpecs().find(type);
            if (dit != defaultRecSpecs().end()) spec = parseRecSpec(dit->second);
        }
        int rs = spec.valid ? spec.size : recSize(type);
        auto oit = g_parseOpts.recSizeOverride.find(type);
        if (oit != g_parseOpts.recSizeOverride.end() && oit->second > 0)
            rs = oit->second;
        if (rs == 0 || p + 4 + (size_t)rs * count > size) break;
        p += 4;
        for (uint16_t i = 0; i < count; ++i) {
            const uint8_t* r = data + p + (size_t)rs * i;
            QmdFace face;
            face.recOff = p + (size_t)rs * i;
            face.recType = type;
            if (!spec.valid) continue;

            RecValues rv = decodeRecord(spec, r);
            // corners: v0..v3 roles; count of v roles decides tri/quad
            int n = 0;
            uint32_t rawIdx[4] = {0, 0, 0, 0};
            for (int j = 0; j < 4; ++j) {
                auto it = rv.v.find("v" + std::to_string(j));
                if (it == rv.v.end()) break;
                rawIdx[j] = it->second >> g_parseOpts.idxShift;
                ++n;
            }
            bool isQuad = (n == 4);
            for (int j = 0; j < n; ++j) {
                uint32_t vi = rawIdx[isQuad ? pm[j] : tm[j]];
                if (vi >= vcount) { face.verts.clear(); break; }
                face.verts.push_back(vbase + vi);
            }
            if (face.verts.empty()) continue;

            // UVs / texture words
            auto getu = [&](const char* role, uint32_t& dst) -> bool {
                auto it = rv.v.find(role);
                if (it == rv.v.end()) return false;
                dst = it->second;
                return true;
            };
            uint32_t dummy;
            face.textured = getu("cba", dummy);
            for (int j = 0; j < n; ++j) {
                int tj = isQuad ? pm[j] : tm[j];
                std::string ur = "u" + std::to_string(tj), wr = "w" + std::to_string(tj);
                auto ui = rv.v.find(ur), wi = rv.v.find(wr);
                if (ui != rv.v.end()) face.uv[j][0] = ui->second & 0xFF;
                if (wi != rv.v.end()) face.uv[j][1] = wi->second & 0xFF;
            }
            getu("cba", dummy); face.clut = rv.v.count("cba") ? rv.v.at("cba") & 0xFFFF : 0;
            face.tpage = rv.v.count("tp") ? rv.v.at("tp") & 0xFFFF : 0;

            // colors: flat trailer word or per-corner roles
            if (rv.v.count("rgb")) {
                uint32_t c = rv.v.at("rgb");
                face.r = c & 0xFF; face.g = (c >> 8) & 0xFF; face.b = (c >> 16) & 0xFF;
            }
            if (rv.v.count("c0")) {
                face.hasVertColors = true;
                for (int j = 0; j < n; ++j) {
                    int tj = isQuad ? pm[j] : tm[j];
                    auto it = rv.v.find("c" + std::to_string(tj));
                    if (it == rv.v.end()) { face.hasVertColors = false; break; }
                    face.vc[j][0] = it->second & 0xFF;
                    face.vc[j][1] = (it->second >> 8) & 0xFF;
                    face.vc[j][2] = (it->second >> 16) & 0xFF;
                }
            }

            // normals: inline s8 triplets (nKa/nKb/nKc roles) or u16 pool
            // indices (n0..n3) — pool lookup only for gouraud types 8/9 with
            // default specs; custom specs are honored as written
            bool poolNrm = nrmPool && rv.v.count("n0") &&
                           (custom || type == 8 || type == 9) &&
                           (g_parseOpts.nrmSource == 2 ||
                                (g_parseOpts.nrmSource != 1 && g_parseOpts.storedNormals));
            if (poolNrm) {
                face.hasNormals = true;
                for (int j = 0; j < n; ++j) {
                    int tj = isQuad ? pm[j] : tm[j];
                    int sraw = (int)(rv.v.at("n" + std::to_string(tj)) >> g_parseOpts.idxShift)
                               + g_parseOpts.nrmIdxOffset;
                    if (sraw < 0 || sraw >= (int)nrmSlots) { face.hasNormals = false; break; }
                    uint32_t sidx = (uint32_t)sraw;
                    float nx, ny, nz;
                    if (s8normals) {
                        static const int kSel[4][3] = {{0,1,2}, {0,1,3}, {0,2,3}, {1,2,3}};
                        const int* bs = kSel[g_parseOpts.nrmByteSel & 3];
                        nx = (int8_t)nrmPool[4 * sidx + bs[0]];
                        ny = (int8_t)nrmPool[4 * sidx + bs[1]];
                        nz = (int8_t)nrmPool[4 * sidx + bs[2]];
                    } else {
                        nx = rds16(nrmPool + 4 * sidx);
                        ny = rds16(nrmPool + 4 * sidx + 2);
                        nz = rds16(f4Pool + 2 * sidx);
                    }
                    float raw3[3] = {nx, ny, nz}, pn[3];
                    mapNormal(raw3, pn);
                    float l = std::sqrt(pn[0]*pn[0] + pn[1]*pn[1] + pn[2]*pn[2]);
                    if (l > 1e-6f) {
                        if (axes)
                            for (int k = 0; k < 3; ++k) face.n[j][k] = pn[axes->src[k]] * axes->sgn[k] / l;
                        else
                            for (int k = 0; k < 3; ++k) face.n[j][k] = pn[k] / l;
                    }
                }
            } else if (rv.v.count("n0a")) {
                face.hasNormals = true;
                for (int j = 0; j < n; ++j) {
                    int tj = isQuad ? pm[j] : tm[j];
                    std::string nb = "n" + std::to_string(tj);
                    float raw3[3] = {(float)(int8_t)(rv.v.at(nb + "a") & 0xFF),
                                     (float)(int8_t)(rv.v.at(nb + "b") & 0xFF),
                                     (float)(int8_t)(rv.v.at(nb + "c") & 0xFF)}, pn[3];
                    mapNormal(raw3, pn);
                    float l = std::sqrt(pn[0]*pn[0] + pn[1]*pn[1] + pn[2]*pn[2]);
                    if (l > 1e-6f) {
                        if (axes)
                            for (int k = 0; k < 3; ++k) face.n[j][k] = pn[axes->src[k]] * axes->sgn[k] / l;
                        else
                            for (int k = 0; k < 3; ++k) face.n[j][k] = pn[k] / l;
                    }
                }
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
                    for (int a = 0; a < 3; ++a) {
                        face.n[j][a] = q.n[cyc[j]][a];
                        face.vc[j][a] = q.vc[cyc[j]][a];
                    }
                }
            }
            // second-uv pool via tail indices (s16 x2, /16 fixed point)
            if (nrmPool && rv.v.count("n0")) {
                bool any2 = false;
                for (int j = 0; j < n; ++j) {
                    int tj = isQuad ? pm[j] : tm[j];
                    int sraw = (int)(rv.v.at("n" + std::to_string(tj)) >> g_parseOpts.idxShift)
                               + g_parseOpts.nrmIdxOffset;
                    if (sraw < 0 || sraw >= (int)nrmSlots) continue;
                    if (s8normals) {
                        face.uv2[j][0] = (uint8_t)std::clamp(rds16(nrmPool + 4 * sraw) / 16, 0, 255);
                        face.uv2[j][1] = (uint8_t)std::clamp(rds16(nrmPool + 4 * sraw + 2) / 16, 0, 255);
                        any2 = true;
                    }
                }
                face.hasUv2 = any2;
                // uv2Mode 1: show the second layer instead of the main uv
                if (g_parseOpts.uv2Mode == 1 && any2) {
                    for (int j = 0; j < n; ++j) {
                        face.uv[j][0] = face.uv2[j][0];
                        face.uv[j][1] = face.uv2[j][1];
                    }
                    face.textured = true;
                }
            }
            // experimental uv transforms
            int fl = uvFlipMode();
            if (fl && face.textured) {
                for (int j = 0; j < n; ++j) {
                    if (fl & 1) std::swap(face.uv[j][0], face.uv[j][1]);
                    if (fl & 2) face.uv[j][0] = 255 - face.uv[j][0];
                    if (fl & 4) face.uv[j][1] = 255 - face.uv[j][1];
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
    out.regions.emplace_back(boff, boff + 0x10, "header");
    int onlyPart = g_parseOpts.onlyPart;
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
        // complex blocks: y from the pair's 2nd half, z from the s16 pool
        // (VERIFIED: hull and hover-skirt are both flat this way; normals
        // (nx=nrm.lo, ny=nrm.hi, nz=f4) match geometric at |dot|=0.997).
        // SB_SWAPYZ=1 forces y-from-pool for A/B tests.
        Axes ax = axesEnv("a,b,c");
        for (uint32_t j = 0; j < count; ++j) {
            int16_t comps[3] = {rds16(data + vb + 4 * j), rds16(data + vb + 4 * j + 2),
                                rds16(data + vc + 2 * j)};
            QmdVertex v;
            v.x = comps[ax.src[0]] * ax.sgn[0];
            v.y = comps[ax.src[1]] * ax.sgn[1];
            v.z = comps[ax.src[2]] * ax.sgn[2];
            sub.verts.push_back(v);
        }
        out.regions.emplace_back(eoff, eoff + 0x24, "part table " + std::to_string(k));
        out.regions.emplace_back(baseOff + xz, baseOff + xz + count * 4, "xz pairs p" + std::to_string(k));
        out.regions.emplace_back(baseOff + y, baseOff + y + count * 2, "y pool p" + std::to_string(k));
        parseChunkStream(data, size, baseOff + prim, 0, count, sub,
                         data + baseOff + nrm, data + baseOff + f4, count, false, &ax);
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
    uint32_t f3 = rd32(base + 0x0C);  // section D (per-vertex s8 normals) or end
    uint32_t f4 = rd32(base + 0x10);  // block end
    out.vertCount = rd32(base + 0x14);

    // vertices: B = n x (s16 x, s16 z), C = n x s16 y
    size_t vb = baseOff + f1, vc = baseOff + f2;
    if (vb + out.vertCount * 4 > size || vc + out.vertCount * 2 > size) return;
    out.verts.reserve(out.vertCount);
    Axes ax = axesEnv("a,c,b");  // simple default: y from the pool
    for (uint32_t i = 0; i < out.vertCount; ++i) {
        int16_t comps[3] = {rds16(data + vb + 4 * i), rds16(data + vb + 4 * i + 2),
                            rds16(data + vc + 2 * i)};
        QmdVertex v;
        v.x = comps[ax.src[0]] * ax.sgn[0];
        v.y = comps[ax.src[1]] * ax.sgn[1];
        v.z = comps[ax.src[2]] * ax.sgn[2];
        out.verts.push_back(v);
    }

    // textured-gouraud blocks carry a per-vertex s8 normal pool between
    // sections D..end, indexed by the record-tail nidx of types 8/9
    out.regions.emplace_back(boff, boff + 0x10 + 0x28, "header");
    out.regions.emplace_back(baseOff + f0, baseOff + f1, "face chunks");
    out.regions.emplace_back(baseOff + f1, baseOff + f2, "xz pairs");
    out.regions.emplace_back(baseOff + f2, baseOff + f3, "y pool");
    if (f3 < f4 && baseOff + f4 <= size)
        out.regions.emplace_back(baseOff + f3, baseOff + f4, "extra/normals");
    // always pass the f3..f4 pool when present; use-sites gate on flags
    bool hasNrmPool = (f3 < f4) && (f4 - f3) == out.vertCount * 4 &&
                      baseOff + f4 <= size;
    parseChunkStream(data, size, baseOff + f0, 0, out.vertCount, out,
                     hasNrmPool ? data + baseOff + f3 : nullptr, nullptr,
                     out.vertCount, /*s8normals=*/true, &ax);
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
