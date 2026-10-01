#!/usr/bin/env python3
"""Silent Bomber ISO injector: writes a modified DATA.BIN back into the
original MODE2/2352 disc image with per-sector EDC/ECC recalculation.

Usage: sb_inject.py <original.bin> <patched_DATA.BIN> [output.bin]
"""
import sys

sys.path.insert(0, '/Users/alexeykrasnopolsky/Desktop/projects/001-PE2Randomizer/tools/pe2-mod-tools/file-manager')
from rom_patcher import RomPatcher

SECTOR_SIZE = 2352
DATA_OFF = 24  # MODE2 data starts at +24
DATA_SIZE = 2048
DATA_BIN_LBA = 436  # from ISO9660 root directory (VERIFIED)

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    src, patched = sys.argv[1], sys.argv[2]
    out_path = sys.argv[3] if len(sys.argv) > 3 else src.replace('.bin', '_modded.bin')

    rom = open(src, 'rb').read()
    data = open(patched, 'rb').read()
    if len(data) != 0x15D8000:
        print('patched DATA.BIN must be %d bytes, got %d' % (0x15D8000, len(data)))
        return 1

    patcher = RomPatcher()
    out = bytearray(rom)
    nsectors = len(data) // DATA_SIZE
    for s in range(nsectors):
        off = (DATA_BIN_LBA + s) * SECTOR_SIZE
        sector = bytearray(rom[off:off + SECTOR_SIZE])
        sector[DATA_OFF:DATA_OFF + DATA_SIZE] = data[s * DATA_SIZE:(s + 1) * DATA_SIZE]
        patcher._eccedc_generate(sector)
        out[off:off + SECTOR_SIZE] = sector
        if s % 2000 == 0:
            print('sector %d/%d...' % (s, nsectors), flush=True)

    open(out_path, 'wb').write(out)
    print('written %s (%d bytes, %d sectors patched)' % (out_path, len(out), nsectors))
    return 0

if __name__ == '__main__':
    sys.exit(main())
