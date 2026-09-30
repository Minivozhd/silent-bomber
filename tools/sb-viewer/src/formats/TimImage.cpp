#include "TimImage.hpp"

#include <cstring>

namespace sb {

static uint16_t rd16(const uint8_t* p) { return p[0] | (p[1] << 8); }
static uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

bool parseTim(const uint8_t* data, size_t size, TimImage& out) {
    if (size < 12) return false;
    if (rd32(data) != 0x10) return false;
    uint32_t flags = rd32(data + 4);
    uint32_t mode = flags & 7;
    out.hasClut = (flags & 8) != 0;
    switch (mode) {
        case 0: out.bpp = 4; break;
        case 1: out.bpp = 8; break;
        case 2: out.bpp = 16; break;
        default: return false;
    }
    if (out.bpp != 16 && !out.hasClut) return false;

    size_t off = 8;
    if (out.hasClut) {
        if (off + 12 > size) return false;
        uint32_t clutLen = rd32(data + off);
        if (clutLen < 12 || off + clutLen > size) return false;
        out.clutX = rd16(data + off + 4);
        out.clutY = rd16(data + off + 6);
        out.clutW = rd16(data + off + 8);
        out.clutH = rd16(data + off + 10);
        if (out.clutW == 0 || out.clutH == 0 || out.clutW > 256 || out.clutH > 256) return false;
        size_t colors = (size_t)out.clutW * out.clutH;
        if (12 + colors * 2 > clutLen) return false;
        out.clut.resize(colors);
        for (size_t i = 0; i < colors; ++i)
            out.clut[i] = rd16(data + off + 12 + i * 2);
        off += clutLen;
    }
    if (off + 12 > size) return false;
    uint32_t dataLen = rd32(data + off);
    if (dataLen < 12 || off + dataLen > size) return false;
    out.imgX = rd16(data + off + 4);
    out.imgY = rd16(data + off + 6);
    uint16_t w = rd16(data + off + 8);
    out.height = rd16(data + off + 10);
    size_t words = (size_t)w * out.height;
    if (12 + words * 2 > dataLen) return false;
    out.vramWords.resize(words);
    for (size_t i = 0; i < words; ++i)
        out.vramWords[i] = rd16(data + off + 12 + i * 2);
    // width in pixels: 16-bit words hold 4 (4bpp), 2 (8bpp) or 1 (16bpp) pixel
    out.width = (out.bpp == 4) ? w * 4 : (out.bpp == 8) ? w * 2 : w;
    return out.valid();
}

static void rgb555(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = (c & 0x1F) << 3;
    g = ((c >> 5) & 0x1F) << 3;
    b = ((c >> 10) & 0x1F) << 3;
}

bool timToRgba(const TimImage& tim, std::vector<uint8_t>& out) {
    if (!tim.valid()) return false;
    out.resize((size_t)tim.width * tim.height * 4);
    for (int y = 0; y < tim.height; ++y) {
        for (int x = 0; x < tim.width; ++x) {
            size_t px = (size_t)y * tim.width + x;
            uint8_t* dst = out.data() + px * 4;
            uint16_t color;
            uint8_t alpha = 255;
            if (tim.bpp == 16) {
                color = tim.vramWords[px];
                if (color == 0) alpha = 0;
            } else {
                uint16_t word = tim.vramWords[px / (tim.bpp == 4 ? 4 : 2)];
                uint8_t idx = (tim.bpp == 4)
                    ? (word >> ((px % 4) * 4)) & 0xF
                    : (word >> ((px % 2) * 8)) & 0xFF;
                if (idx == 0) alpha = 0;
                if (idx >= tim.clut.size()) idx = 0;
                color = tim.clut.empty() ? 0 : tim.clut[idx];
            }
            rgb555(color, dst[0], dst[1], dst[2]);
            dst[3] = alpha;
        }
    }
    return true;
}

std::vector<size_t> findTims(const uint8_t* data, size_t size) {
    std::vector<size_t> hits;
    for (size_t i = 0; i + 12 <= size; ++i) {
        if (data[i] != 0x10 || data[i + 1] || data[i + 2] || data[i + 3]) continue;
        uint32_t flags = rd32(data + i + 4);
        if ((flags & ~0x0Fu) != 0) continue;  // only low 4 bits are meaningful
        TimImage t;
        if (parseTim(data + i, size - i, t)) hits.push_back(i);
    }
    return hits;
}

} // namespace sb
