/**
 * @file TextureBank.hpp
 * @brief Virtual-VRAM texture access for QMD textured faces.
 *
 * Builds the package's texture memory from the part1 TIM bundle: each TIM
 * contributes an image rect (img_x/img_y, halfword coords) and CLUT rect(s).
 * Faces address texels as (tpage_x*64 + u/2, tpage_y*256 + v) for 4bpp and
 * (tpage_x*64 + u, tpage_y*256 + v) for 8bpp; the palette line comes from the
 * face's clut word (y line, x*16 colors).
 */
#pragma once

#include <cstdint>
#include <vector>
#include "TimImage.hpp"

namespace sb {

struct TextureBank {
    struct Page {
        const TimImage* tim = nullptr;
        int bpp = 0;
    };
    std::vector<TimImage> tims;

    // Global VRAM palette memory (1024 x 512 halfwords): every TIM's CLUT
    // block is written at its (clutX, clutY) rect; faces address colors by
    // (cba.x*16 + idx, cba.y) regardless of which TIM uploaded them.
    std::vector<uint16_t> clutMem;
    std::vector<uint8_t> clutValid;

    void load(const uint8_t* data, size_t size);

    /// Finds the TIM whose image rect contains halfword (x, y), or nullptr.
    const TimImage* pageAt(int xHw, int y) const;

    /// Sample a texel: x in halfwords, y in lines, palette line from the face
    /// clut word (clut_y - tim->clutY). Returns RGBA; alpha 0 when transparent
    /// (palette index 0) or outside any page.
    void sample(int xHw, int y, uint16_t clutWord, uint8_t out[4]) const;
};

} // namespace sb
