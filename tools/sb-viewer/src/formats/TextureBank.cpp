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
    // global VRAM palette memory (1024 x 512 halfwords): CLUTs are shared
    // between TIMs (e.g. CMFANR00's faces sample TIM6's image with TIM7-9's
    // palette), so they must be addressable globally, not per-TIM.
    clutMem.assign(1024 * 512, 0);
    clutValid.assign(1024 * 512, 0);
    for (const auto& t : tims) {
        if (t.clut.empty() || t.clutW == 0 || t.clutH == 0) continue;
        for (int y = 0; y < t.clutH; ++y) {
            for (int x = 0; x < t.clutW; ++x) {
                int gx = t.clutX + x, gy = t.clutY + y;
                if (gx >= 1024 || gy >= 512) continue;
                size_t dst = (size_t)gy * 1024 + gx;
                clutMem[dst] = t.clut[(size_t)y * t.clutW + x];
                clutValid[dst] = 1;
            }
        }
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
        // palette: global VRAM CLUT memory — face cba addresses it directly
        // (x in 16-color units, y in lines), independent of the image's TIM
        int clutY = (clutWord >> 6) & 0x1FF;
        int clutX = (clutWord & 0x3F) * 16 + idx;
        if (clutY >= 512 || clutX >= 1024) return;
        size_t pal = (size_t)clutY * 1024 + clutX;
        if (!clutValid[pal]) return;
        rgb555(clutMem[pal], out[0], out[1], out[2]);
        out[3] = 255;
        return;
    }
}

} // namespace sb
