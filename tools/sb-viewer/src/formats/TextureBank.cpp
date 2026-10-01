#include "TextureBank.hpp"

namespace sb {

void TextureBank::load(const uint8_t* data, size_t size) {
    tims.clear();
    auto offs = findTims(data, size);
    for (size_t o : offs) {
        TimImage t;
        if (parseTim(data + o, size - o, t))
            tims.push_back(std::move(t));
    }
}

const TimImage* TextureBank::pageAt(int xHw, int y) const {
    for (const auto& t : tims) {
        int wordsPerLine = (t.bpp == 4) ? t.width / 4 : (t.bpp == 8) ? t.width / 2 : t.width;
        if (xHw >= t.imgX && xHw < t.imgX + wordsPerLine &&
            y >= t.imgY && y < t.imgY + t.height)
            return &t;
    }
    return nullptr;
}

static inline void rgb555(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = (c & 0x1F) << 3;
    g = ((c >> 5) & 0x1F) << 3;
    b = ((c >> 10) & 0x1F) << 3;
}

void TextureBank::sample(int xPx, int y, uint16_t clutWord, uint8_t out[4]) const {
    out[0] = out[1] = out[2] = 0;
    out[3] = 0;
    for (const TimImage& t : tims) {
        int wordsPerLine = (t.bpp == 4) ? t.width / 4 : (t.bpp == 8) ? t.width / 2 : t.width;
        int pxPerWord = (t.bpp == 4) ? 4 : (t.bpp == 8) ? 2 : 1;
        int relX = xPx - t.imgX * pxPerWord;
        int relY = y - t.imgY;
        if (relX < 0 || relX >= t.width || relY < 0 || relY >= t.height)
            continue;
        uint16_t word = t.vramWords[(size_t)relY * wordsPerLine + relX / pxPerWord];
        uint8_t idx = 0;
        if (t.bpp == 16) {
            if (word == 0) return;
            rgb555(word, out[0], out[1], out[2]);
            out[3] = 255;
            return;
        }
        if (t.bpp == 4)
            idx = (word >> ((relX % 4) * 4)) & 0xF;
        else
            idx = (word >> ((relX % 2) * 8)) & 0xFF;
        if (idx == 0) return;
        // palette entry: face clut word (y at bits 6..15 lines, x at 0..5 in
        // 16-color units) relative to the TIM's clut rect (clutX in halfwords)
        int clutLine = ((clutWord >> 6) & 0x3FF) - t.clutY;
        int colIdx = (clutWord & 0x3F) * 16 + idx - t.clutX;
        size_t palette = (size_t)clutLine * t.clutW + colIdx;
        if (clutLine < 0 || colIdx < 0 || palette >= t.clut.size()) return;
        rgb555(t.clut[palette], out[0], out[1], out[2]);
        out[3] = 255;
        return;
    }
}

} // namespace sb
