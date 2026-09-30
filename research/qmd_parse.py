#!/usr/bin/env python3
"""Silent Bomber (PS1) QMD model parser + software renderer. Pure stdlib.

Format reverse-engineered from DATA.BIN packages (see qmd_format.md):

part3 container: "pQES" magic + data blob, then a chain of QMD blocks.
Unit packs (grpA_XX.part2_raw.bin): bare chain of QMD blocks (no pQES).

QMD block:
  +0x00  "QMD " + 8-byte name
  +0x0C  u32 kind:  (1,0)/(1,1) as u16 pair -> "simple" model block
                    lo = part count N > 1     -> "complex" block (part table)
  --- simple block (w0c.lo == 1) ---
  +0x10  u32 f0  offset of section A (faces)      relative to +0x10
  +0x14  u32 f1  offset of section B (vertices: s16 x, s16 z per vertex)
  +0x18  u32 f2  offset of section C (s16 y per vertex)
  +0x1C  u32 f3  offset of block end (== next block - (block+0x10))
  +0x20  u32 f4  == f3 (duplicated)
  +0x24  u32 f5  vertex count
  +0x28  u32 f6  offset to related (animation?) data in the same file [UNRESOLVED]
  +0x2C  u32 g0, +0x30 u32 g1                   [UNRESOLVED]
  section A: chunks: u16 count, u16 type; type 0 = 4-index polys (36B rec),
             type 1 = tris gouraud (28B), type 3 = tris flat (20B); u32 0 = end.
             vertex indices are stored x2 (u16 >> 1).
  --- complex block (w0c.lo = N parts) ---
  +0x10  N x 0x24-byte part entries:
         +0x00 u32 f0: prim-stream payload offset (rel. block+0x10)
         +0x04 u32 f1: slice offset -> s16 pairs pool  (x,z?)
         +0x08 u32 f2: slice offset -> s16 pool (y?)
         +0x0C u32 f3: slice offset -> normals pool (s16, ~4096 scale)
         +0x10 u32 f4: slice offset -> second ~4096-scale pool
         +0x14 u32 count
         +0x18 u32 f6
         +0x1C 4 x s16: bounding sphere (cx,cy,cz,r) [part local space]
  prim stream (global, contiguous [u32 rgb+cmd][payload]):
     cmd 0x3c -> 4 (v,n) u16 pairs  + uv0,uv1,uv2,cba,uv3,tpage  (32B total)
     cmd 0x34 -> 3 or 6 (v,n) pairs + uv0,uv1,uv2,cba,pad,tpage  (28B/40B)
     cba/tpage are literal PS1 GPU words (VERIFIED against part1 TIM rects).

Usage:
  qmd_parse.py <file>                 -> block inventory + per-block stats
  qmd_parse.py render                 -> render demo set into render/
"""
import struct, sys, os, math, zlib

def u16(b, o): return struct.unpack_from('<H', b, o)[0]
def s16(b, o): return struct.unpack_from('<h', b, o)[0]
def u32(b, o): return struct.unpack_from('<I', b, o)[0]

# ----------------------------------------------------------------- blocks
class Block:
    def __init__(self, buf, off):
        self.buf, self.off = buf, off
        assert buf[off:off+4] == b'QMD '
        self.name = buf[off+4:off+0x0C].decode('ascii', 'replace')
        w = u32(buf, off+0x0C)
        self.kind_lo, self.kind_hi = w & 0xFFFF, w >> 16
        self.base = off + 0x10

    @property
    def is_simple(self): return self.kind_lo == 1

    # ---- simple block ----
    def hdr(self):
        f = [u32(self.buf, self.base + 4*i) for i in range(7)]
        g = [u32(self.buf, self.base + 0x1C + 4*i) for i in range(2)]
        return f, g

    def end(self):
        if self.is_simple:
            return self.base + u32(self.buf, self.base + 0x0C)  # f3
        return None

    def nverts(self):
        return u32(self.buf, self.base + 0x14) if self.is_simple else 0

    def vertices(self):
        """simple block: B = n x (s16 x, s16 z), C = n x s16 y."""
        f, _ = self.hdr()
        b, c, n = self.base + f[1], self.base + f[2], f[5]
        return [(s16(self.buf, b+4*i), s16(self.buf, c+2*i), s16(self.buf, b+4*i+2))
                for i in range(n)]

    # Face chunk record sizes (measured 2026-09-30 by chunk-landing analysis):
    #   type 0 = quad, per-vertex normals+colors, 36B  [VERIFIED]
    #   type 1 = tri,  per-vertex normals+colors, 28B  [VERIFIED]
    #   type 2 = quad, per-vertex normals, 1 color, 24B [VERIFIED]
    #   type 3 = tri,  per-vertex normals, 1 color, 20B [VERIFIED]
    #   type 4 = quad, colors only (no normals), 24B   [VERIFIED size]
    #   type 5 = tri,  colors only (no normals), 24B   [VERIFIED size]
    #   type 6 = two-quad variant, 28B                 [VERIFIED size]
    #   type 8 = quad textured gouraud (GT4, cmd 0x3C), 42B [VERIFIED size]
    #   type 10 = quad textured flat (FT4, cmd 0x2C), 36B    [VERIFIED size]
    REC_SIZE = {0: 36, 1: 28, 2: 24, 3: 20, 4: 24, 5: 24, 6: 28, 8: 42, 10: 36}

    def faces(self):
        """simple block section A -> list of polys:
        (vidx list, kind, colors list, rec offset)."""
        f, _ = self.hdr()
        p, end = self.base + f[0], self.base + f[1]
        out = []
        while p + 4 <= end:
            count, typ = u16(self.buf, p), u16(self.buf, p+2)
            if count == 0 and typ == 0:
                break
            p += 4
            sz = self.REC_SIZE[typ]
            for i in range(count):
                r = p + sz*i
                if typ == 0:      # 4-index poly (drawn as 2 tris), 4 normals, 4 colors
                    idx = [u16(self.buf, r+2*j) >> 1 for j in range(4)]
                    nrm = [tuple(struct.unpack_from('3b', self.buf, r+8+3*j)) for j in range(4)]
                    col = [tuple(self.buf[r+20+4*j:r+24+4*j]) for j in range(4)]
                    out.append((idx, 'p4', col, r))
                elif typ == 2:    # quad, 4 normals, 1 color
                    idx = [u16(self.buf, r+2*j) >> 1 for j in range(4)]
                    col = [tuple(self.buf[r+20:r+24])]
                    out.append((idx, 'p4-f', col, r))
                elif typ == 4:    # quad, 4 colors, no normals
                    idx = [u16(self.buf, r+2*j) >> 1 for j in range(4)]
                    col = [tuple(self.buf[r+8+4*j:r+12+4*j]) for j in range(4)]
                    out.append((idx, 'p4-c', col, r))
                elif typ == 1:    # tri, 3 normals(+pad), 3 colors
                    idx = [u16(self.buf, r+2*j) >> 1 for j in range(3)]
                    nrm = [tuple(struct.unpack_from('3b', self.buf, r+6+3*j)) for j in range(3)]
                    col = [tuple(self.buf[r+16+4*j:r+20+4*j]) for j in range(3)]
                    out.append((idx, 'tri-g', col, r))
                elif typ == 3:    # tri, 3 normals(+pad), 1 color
                    idx = [u16(self.buf, r+2*j) >> 1 for j in range(3)]
                    col = [tuple(self.buf[r+16:r+20])]
                    out.append((idx, 'tri-f', col, r))
                elif typ == 5:    # tri, 3 colors, no normals
                    idx = [u16(self.buf, r+2*j) >> 1 for j in range(3)]
                    col = [tuple(self.buf[r+6+4*j:r+10+4*j]) for j in range(3)]
                    out.append((idx, 'tri-c', col, r))
                elif typ == 6:    # two-quad strip record: two quads, colors at tail
                    idx = [u16(self.buf, r+2*j) >> 1 for j in range(4)]
                    col = [tuple(self.buf[r+24:r+28])]
                    out.append((idx, 'p4-s', col, r))
                elif typ == 8:    # quad textured gouraud: idx, nrm, col, uv, cba, tpage
                    idx = [u16(self.buf, r+2*j) >> 1 for j in range(4)]
                    col = [tuple(self.buf[r+20+4*j:r+24+4*j]) for j in range(4)]
                    out.append((idx, 'p4-t', col, r))
                elif typ == 10:   # quad textured flat: idx, nrm, col, uv, cba, tpage
                    idx = [u16(self.buf, r+2*j) >> 1 for j in range(4)]
                    col = [tuple(self.buf[r+20:r+24])]
                    out.append((idx, 'p4-tf', col, r))
                else:
                    break
            p += sz*count
        return out

    # ---- complex block ----
    def parts(self):
        """complex block -> list of part entries."""
        n = self.kind_lo
        out = []
        for i in range(n):
            e = self.base + 0x24*i
            f = [u32(self.buf, e+4*j) for j in range(7)]
            bbox = [s16(self.buf, e+0x1C+2*j) for j in range(4)]
            out.append({'prim': self.base + f[0], 'xz': self.base + f[1],
                        'y': self.base + f[2], 'nrm': self.base + f[3],
                        'f4': self.base + f[4], 'count': f[5], 'f6': f[6],
                        'bbox': bbox})
        return out

    def prim_records(self):
        """complex block: global prim stream of GPU-like records.
        Starts right after the part table; ends at the min f1 target.
        Returns list of dicts with v/n index pairs, uvs, cba, tpage."""
        parts = self.parts()
        start = self.base + 0x24*len(parts)
        stop = min(p['xz'] for p in parts)
        out = []
        p = start
        first = True
        while p + 4 <= stop - 0:  # stream ends with a 16B cmd+3-pair trailer
            if first:
                # first record has no rgb+cmd word
                npairs, psz = 3, 24
                rgb, cmd = (0x80, 0x80, 0x80), 0x34
                hdr_len = 0
                first = False
            else:
                rgb = tuple(self.buf[p:p+3]); cmd = self.buf[p+3]
                hdr_len = 4
                if cmd == 0x3c:
                    cba_at = 26
                    npairs = 4
                elif cmd == 0x34:
                    cba_at = 22
                    npairs = 3
                else:
                    break
                if u16(self.buf, p+hdr_len+cba_at-4) != 0x3E37 and \
                   u16(self.buf, p+hdr_len+cba_at+6) == 0x3E37:
                    npairs, cba_at = 6, 34  # 6-pair variant
                if cmd not in (0x34, 0x3c):
                    break
            q = p + hdr_len
            pairs = [(u16(self.buf, q+4*j), u16(self.buf, q+4*j+2)) for j in range(npairs)]
            uv_off = q + 4*npairs
            uvs = [tuple(self.buf[uv_off+2*j:uv_off+2*j+2]) for j in range(3)]
            cba = u16(self.buf, uv_off+6)
            uv3 = tuple(self.buf[uv_off+8:uv_off+10])
            tpage = u16(self.buf, uv_off+10)
            out.append({'off': p, 'cmd': cmd, 'rgb': rgb, 'pairs': pairs,
                        'uvs': uvs, 'cba': cba, 'uv3': uv3, 'tpage': tpage})
            p = uv_off + 12
        return out

def iter_blocks(buf):
    import re
    for m in re.finditer(b'QMD ', buf):
        yield Block(buf, m.start())

# ----------------------------------------------------------------- PNG
def write_png(path, w, h, rgb):
    def chunk(tag, data):
        c = tag + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xFFFFFFFF)
    raw = b''.join(b'\x00' + rgb[y*w*3:(y+1)*w*3] for y in range(h))
    open(path, 'wb').write(b'\x89PNG\r\n\x1a\n'
                           + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
                           + chunk(b'IDAT', zlib.compress(raw, 6))
                           + chunk(b'IEND', b''))

# ----------------------------------------------------------------- renderer
def render(verts, tris, path, size=512, colors=None):
    """orthographic, painter+zbuffer hybrid (zbuffer), flat shading; tris = [i0,i1,i2]."""
    if not verts:
        return
    ang, tilt = math.radians(35), math.radians(-30)
    ca, sa, ct, st = math.cos(ang), math.sin(ang), math.cos(tilt), math.sin(tilt)
    pv = []
    for x, y, z in verts:
        x1 = x*ca + z*sa
        z1 = -x*sa + z*ca
        pv.append((x1, y*ct - z1*st, y*st + z1*ct))
    xs = [p[0] for p in pv]; ys = [p[1] for p in pv]
    sc = (size-40) / max(max(xs)-min(xs), max(ys)-min(ys), 1e-9)
    cx, cy = (max(xs)+min(xs))/2, (max(ys)+min(ys))/2
    img = bytearray(b'\x18\x18\x20' * size * size)
    zb = [-1e30]*(size*size)

    def fill(poly, col):
        ys2 = [p[1] for p in poly]
        for py in range(max(0, int(min(ys2))), min(size, int(max(ys2))+1)):
            xs2 = []
            for i in range(len(poly)):
                a, bq = poly[i], poly[(i+1) % len(poly)]
                if (a[1] <= py) != (bq[1] <= py):
                    t = (py - a[1]) / (bq[1] - a[1] + 1e-12)
                    xs2.append((a[0] + t*(bq[0]-a[0]), a[2] + t*(bq[2]-a[2])))
            xs2.sort()
            for k in range(0, len(xs2)-1, 2):
                xa, xb = xs2[k], xs2[k+1]
                for px in range(max(0, int(xa[0])), min(size, int(xb[0])+1)):
                    t = (px - xa[0]) / (xb[0] - xa[0] + 1e-12)
                    z = xa[1] + t*(xb[1]-xa[1])
                    if 0 <= px < size and 0 <= py < size:
                        j = py*size+px
                        if z > zb[j]:
                            zb[j] = z
                            img[j*3:j*3+3] = bytes(col)

    for fi, idx in enumerate(tris):
        if any(i >= len(verts) for i in idx):
            continue
        pts = [(int((pv[i][0]-cx)*sc + size/2), int((pv[i][1]-cy)*sc + size/2), pv[i][2])
               for i in idx]
        a, b, c = (verts[idx[0]], verts[idx[1]], verts[idx[2]])
        ux, uy, uz = b[0]-a[0], b[1]-a[1], b[2]-a[2]
        vx, vy, vz = c[0]-a[0], c[1]-a[1], c[2]-a[2]
        nx, ny, nz = uy*vz-uz*vy, uz*vx-ux*vz, ux*vy-uy*vx
        ln = math.sqrt(nx*nx+ny*ny+nz*nz) or 1
        lam = max(0.15, min(1.0, (nx*0.4+ny*0.8+nz*0.45)/ln*0.5+0.55))
        base = colors[fi] if colors else (200, 170, 90)
        fill(pts, tuple(min(255, int(cc*lam)) for cc in base))
    write_png(path, size, size, bytes(img))

def simple_tris(faces):
    """split 4-index polys into 2 tris each (v0,v1,v2)+(v1,v2,v3) [VERIFIED on X1LJUT00]."""
    tris, cols = [], []
    for idx, kind, col, off in faces:
        if len(idx) == 4:
            tris.append([idx[0], idx[1], idx[2]]); cols.append(col[0][:3])
            tris.append([idx[1], idx[2], idx[3]]); cols.append(col[min(2, len(col)-1)][:3])
        else:
            tris.append(list(idx)); cols.append(col[0][:3])
    return tris, cols

def export_obj(path, verts, tris, note=''):
    with open(path, 'w') as f:
        f.write(f'# Silent Bomber QMD {note}\n')
        for v in verts:
            f.write(f'v {v[0]} {v[1]} {v[2]}\n')
        for t in tris:
            f.write('f ' + ' '.join(str(i+1) for i in t) + '\n')

# ----------------------------------------------------------------- complex render (best effort)
def complex_render(block, path):
    """HYPOTHESIS: v -> per-part? global s16 (x,z) pool at f1 slices, y from f2 pool.
    Currently renders the global stream with global pools [UNRESOLVED mapping]."""
    parts = block.parts()
    xz0 = min(p['xz'] for p in parts)
    y0 = min(p['y'] for p in parts)
    nrm0 = min(p['nrm'] for p in parts)
    f40 = min(p['f4'] for p in parts)
    xz_n = (nrm0 - xz0) // 4
    y_n = (f40 - y0) // 2
    xz = [(s16(block.buf, xz0+4*i), s16(block.buf, xz0+4*i+2)) for i in range(xz_n)]
    yy = [s16(block.buf, y0+2*i) for i in range(y_n)]
    verts, tris = [], []
    vmap = {}
    for rec in block.prim_records():
        idx = []
        for v, n in rec['pairs']:
            vi = v >> 1
            if vi >= len(xz):
                vi = 0
            key = vi
            if key not in vmap:
                vmap[key] = len(verts)
                y = yy[vi] if vi < len(yy) else 0
                verts.append((xz[vi][0], y, xz[vi][1]))
            idx.append(vmap[key])
        if len(idx) == 4:
            tris += [[idx[0], idx[1], idx[2]], [idx[1], idx[2], idx[3]]]
        elif len(idx) == 6:
            tris += [[idx[0], idx[1], idx[2]], [idx[3], idx[4], idx[5]]]
        else:
            tris.append(idx)
    render(verts, tris, path)
    return len(verts), len(tris)

# ----------------------------------------------------------------- CLI
def inventory(path):
    buf = open(path, 'rb').read()
    print(f'{os.path.basename(path)}: {len(buf):#x} bytes, pqes={buf[:4] == b"pQES"}')
    for b in iter_blocks(buf):
        if b.is_simple:
            f, g = b.hdr()
            vs = b.vertices()
            fa = b.faces()
            kinds = {}
            for _, k, _, _ in fa:
                kinds[k] = kinds.get(k, 0) + 1
            print(f'  @{b.off:#07x} {b.name} SIMPLE f0={f[0]:#x} f1={f[1]:#x} f2={f[2]:#x} '
                  f'f3=f4={f[3]:#x} nv={f[5]} f6={f[6]:#x} g={[hex(x) for x in g]} '
                  f'polys={len(fa)} {kinds} end={b.end():#x}')
        else:
            parts = b.parts()
            recs = b.prim_records()
            from collections import Counter
            cc = Counter((r['cmd'], len(r['pairs'])) for r in recs)
            print(f'  @{b.off:#07x} {b.name} COMPLEX parts={len(parts)} primrecs={len(recs)} {dict(cc)}')
            for i, p in enumerate(parts):
                print(f'      part{i}: prim+{p["prim"]-b.base:#06x} xz+{p["xz"]-b.base:#06x} '
                      f'y+{p["y"]-b.base:#06x} nrm+{p["nrm"]-b.base:#06x} f4+{p["f4"]-b.base:#06x} '
                      f'cnt={p["count"]} f6={p["f6"]:#x} bbox={p["bbox"]}')

def render_demo(here):
    out = os.path.join(here, 'out')
    rd = os.path.join(here, 'render')
    os.makedirs(rd, exist_ok=True)
    # scene block SMP01B01 (P01)
    buf = open(os.path.join(out, 'P01.part3_raw.bin'), 'rb').read()
    b = Block(buf, 0x924)
    vs = b.vertices()
    tris, cols = simple_tris(b.faces())
    render(vs, tris, os.path.join(rd, 'SMP01B01.png'))
    export_obj(os.path.join(rd, 'SMP01B01.obj'), vs, tris, 'SMP01B01 P01.part3')
    print('SMP01B01:', len(vs), 'verts', len(tris), 'tris')
    # GMTANK01 (P01) - simple block tank
    b = Block(buf, 0x3b51c)
    vs = b.vertices()
    tris, cols = simple_tris(b.faces())
    render(vs, tris, os.path.join(rd, 'GMTANK01.png'))
    print('GMTANK01:', len(vs), 'verts', len(tris), 'tris')
    # unit pack grpA_00: X1LJUT00 (simple) + C1LJUT00 (complex)
    buf2 = open(os.path.join(out, 'grpA_00.part2_raw.bin'), 'rb').read()
    b = Block(buf2, 0)
    vs = b.vertices()
    tris, cols = simple_tris(b.faces())
    render(vs, tris, os.path.join(rd, 'X1LJUT00.png'))
    export_obj(os.path.join(rd, 'X1LJUT00.obj'), vs, tris, 'X1LJUT00 grpA_00.part2')
    print('X1LJUT00:', len(vs), 'verts', len(tris), 'tris')
    b = Block(buf2, 0x2DC)
    nv, nt = complex_render(b, os.path.join(rd, 'C1LJUT00.png'))
    print('C1LJUT00 (best-effort):', nv, 'verts', nt, 'tris')

if __name__ == '__main__':
    here = os.path.dirname(os.path.abspath(__file__))
    if len(sys.argv) > 1 and sys.argv[1] == 'render':
        render_demo(here)
    elif len(sys.argv) > 1:
        inventory(sys.argv[1])
    else:
        inventory(os.path.join(here, 'out', 'P01.part3_raw.bin'))
