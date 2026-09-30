/**
 * @file TimImage.hpp
 * @brief PlayStation TIM image parsing (4bpp/8bpp with CLUT, 16bpp direct).
 *
 * TIM layout (little-endian):
 *   u32 magic  = 0x10
 *   u32 flags  (bpp: 0x00=4bpp, 0x01=8bpp, 0x02=16bpp; bit 3 (0x08) = CLUT present)
 *   if CLUT: u32 clut_block_len (incl. this field) then:
 *            u16 clut_x, u16 clut_y, u16 clut_w, u16 clut_h, then w*h u16 colors (RGB555)
 *   u32 data_block_len (incl. this field)
 *   u16 img_x, u16 img_y, u16 img_w (in 16-bit words), u16 img_h
 *   pixel data: img_w*img_h*2 bytes
 */
#pragma once

#include <cstdint>
#include <vector>

namespace sb {

struct TimImage {
    int width = 0;       // pixels
    int height = 0;
    int bpp = 0;         // 4, 8, 16
    bool hasClut = false;
    std::vector<uint16_t> clut;       // RGB555 entries
    std::vector<uint16_t> vramWords;  // raw image data as u16 halfwords
    uint16_t clutX = 0, clutY = 0, clutW = 0, clutH = 0;
    uint16_t imgX = 0, imgY = 0;

    bool valid() const { return width > 0 && height > 0; }
};

/// Parses a TIM image from raw bytes. Returns false when no valid TIM found.
bool parseTim(const uint8_t* data, size_t size, TimImage& out);

/// Decodes to RGBA32. Palette index 0 maps to alpha 0 (transparent).
/// Out buffer is width*height*4. Returns false on invalid input.
bool timToRgba(const TimImage& tim, std::vector<uint8_t>& out);

/// Finds all TIM images inside a buffer (e.g. a decoded part1 bundle).
/// Returns the byte offsets of every valid TIM.
std::vector<size_t> findTims(const uint8_t* data, size_t size);

} // namespace sb
