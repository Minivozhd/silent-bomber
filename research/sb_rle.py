#!/usr/bin/env python3
"""Silent Bomber RLE decompressor — reverse-engineered from func_80012BB0/func_80012AEC (SLUS_009.02).

Stream format (driver func_80012AEC):
    repeat:
        u32 skip      ; if 0 -> end of stream
        block:
            u32 csize   ; compressed size INCLUDING this 10-byte block header
            u32 dsize   ; decompressed size
            u8  escape  ; escape value (nibble or byte depending on mode)
            u8  mode    ; 0 = nibble RLE, !=0 = byte RLE
            u8  data[csize-10]
        pad to 4 (skip*4 == align4(csize)); blocks concatenate in output.

Block payload (func_80012BB0):
  nibble mode: read nibbles high-first. nibble != escape -> literal nibble out
    (high nibble first in output). nibble == escape -> read (count, value) nibble
    pair, emit value nibble `count` times (count==0 -> emit nothing).
  byte mode: byte != escape -> literal. byte == escape -> read count byte then
    value byte, emit value `count` times (count==0 -> emit nothing).
"""


def decompress_block(block: bytes, dsize: int) -> bytes:
    csize = int.from_bytes(block[0:4], "little")
    esc = block[8]
    mode = block[9]
    data = block[10:csize]
    out = bytearray()
    if mode == 0:
        # nibble mode
        pos = 0           # byte index into data
        hi = True         # next nibble is high
        out_hi = True     # next output nibble is high
        cur = 0           # current output byte being assembled
        n = len(data)

        def get_nibble():
            nonlocal pos, hi
            b = data[pos]
            if hi:
                return (b >> 4) & 0xF
            else:
                pos += 1
                return b & 0xF

        while pos < n:
            nib = get_nibble()
            hi = not hi
            if nib != esc:
                if out_hi:
                    cur = nib << 4
                else:
                    out.append(cur | nib)
                out_hi = not out_hi
            else:
                if pos >= n:  # escape was the very last (low) nibble: asm returns here
                    break
                cnt = get_nibble()
                hi = not hi
                val = get_nibble()
                hi = not hi
                for _ in range(cnt):
                    if out_hi:
                        cur = val << 4
                    else:
                        out.append(cur | val)
                    out_hi = not out_hi
    else:
        pos = 0
        n = len(data)
        while pos < n:
            b = data[pos]
            pos += 1
            if b != esc:
                out.append(b)
            else:
                if pos >= n:
                    break
                cnt = data[pos]
                pos += 1
                if cnt == 0:
                    continue
                val = data[pos]
                pos += 1
                out.extend(bytes([val]) * cnt)
    return bytes(out)


def decompress_stream(buf: bytes, max_out: int = 1 << 28):
    """Decompress a func_80012AEC stream starting at buf[0]. Returns (output, blocks, bytes_consumed)."""
    p = 0
    out = bytearray()
    blocks = []
    while True:
        skip = int.from_bytes(buf[p:p + 4], "little")
        if skip == 0:
            p += 4
            break
        p += 4
        block = buf[p:p + skip * 4]
        csize = int.from_bytes(block[0:4], "little")
        dsize = int.from_bytes(block[4:8], "little")
        dec = decompress_block(block, dsize)
        # dsize is what the driver uses to advance dst; actual output may be a
        # couple bytes short (trailing truncated escape) or one byte over
        # (odd trailing nibble). Truncate/pad like the hardware effectively does.
        if len(dec) > dsize:
            dec = dec[:dsize]
        elif len(dec) < dsize:
            dec = dec + b"\x00" * (dsize - len(dec))
        blocks.append((p, csize, dsize, block[8], block[9]))
        out.extend(dec)
        p += skip * 4
        if len(out) > max_out:
            raise RuntimeError("runaway")
    return bytes(out), blocks, p


if __name__ == "__main__":
    import sys
    d = open("/Users/alexeykrasnopolsky/Desktop/silent_bomber/DATA.BIN", "rb").read()
    # 1) the header table stream at offset 0
    tab, blocks, consumed = decompress_stream(d[0:0x2B38])
    print(f"table stream: {len(blocks)} block(s), decompressed {len(tab):#x} bytes, consumed {consumed:#x}")
    for b in blocks:
        print("  block @%#x csize=%#x dsize=%#x esc=%#x mode=%d" % b)
    open("/Users/alexeykrasnopolsky/Desktop/silent_bomber/experiments/out/file_table.bin", "wb").write(tab)
    # hexdump start of table
    for i in range(0, 0x100, 16):
        print(f"{i:06x}  " + " ".join(f"{x:02x}" for x in tab[i:i+16]))
