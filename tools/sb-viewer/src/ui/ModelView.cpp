#include "ModelView.hpp"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

ModelView::ModelView(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ModelView::clear() {
    m_verts.clear();
    m_faces.clear();
    m_img = QImage();
    update();
}

void ModelView::setModel(const std::vector<sb::QmdVertex>& verts,
                         const std::vector<sb::QmdFace>& faces,
                         const sb::TextureBank* tex) {
    m_verts = verts;
    m_faces = faces;
    m_tex = tex;
    render();
}

void ModelView::mousePressEvent(QMouseEvent* e) {
    m_dragging = true;
    m_lastPos = e->pos();
}

void ModelView::mouseMoveEvent(QMouseEvent* e) {
    if (!m_dragging) return;
    QPoint d = e->pos() - m_lastPos;
    m_lastPos = e->pos();
    m_yaw += d.x() * 0.01f;
    m_pitch = std::max(-1.55f, std::min(1.55f, m_pitch + d.y() * 0.01f));
    render();
}

void ModelView::wheelEvent(QWheelEvent* e) {
    m_zoom *= (e->angleDelta().y() > 0) ? 1.1f : 0.9f;
    render();
}

void ModelView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(24, 24, 32));
    if (!m_img.isNull())
        p.drawImage(rect().topLeft(), m_img.scaled(size(), Qt::KeepAspectRatio, Qt::FastTransformation));
}

void ModelView::render() {
    int W = std::max(320, width()), H = std::max(240, height());
    m_img = QImage(W, H, QImage::Format_RGB32);
    m_img.fill(QColor(24, 24, 32));
    if (m_verts.empty()) { update(); return; }

    // orbit transform
    float ca = std::cos(m_yaw), sa = std::sin(m_yaw);
    float cp = std::cos(m_pitch), sp = std::sin(m_pitch);
    // optional pre-rotation around X (SB_ROTX, degrees) — model-space pitch
    // correction for assets authored "standing up" (e.g. EMBTNK00 boss tank)
    float rxDeg = [] { const char* s = getenv("SB_ROTX"); return s ? atof(s) : 0.0; }();
    float rx = (float)(rxDeg * 3.14159265358979 / 180.0);
    float crx = std::cos(rx), srx = std::sin(rx);
    std::vector<std::array<float,3>> pv(m_verts.size());
    float mn[3] = {1e30f, 1e30f, 1e30f}, mx[3] = {-1e30f, -1e30f, -1e30f};
    for (size_t i = 0; i < m_verts.size(); ++i) {
        const auto& v = m_verts[i];
        float vy = v.y * crx - v.z * srx;
        float vz = v.y * srx + v.z * crx;
        float x1 = v.x * ca + vz * sa;
        float z1 = -v.x * sa + vz * ca;
        float y2 = vy * cp - z1 * sp;
        float z2 = vy * sp + z1 * cp;
        pv[i] = {x1, y2, z2};
        mn[0] = std::min(mn[0], x1); mx[0] = std::max(mx[0], x1);
        mn[1] = std::min(mn[1], y2); mx[1] = std::max(mx[1], y2);
        mn[2] = std::min(mn[2], z2); mx[2] = std::max(mx[2], z2);
    }
    float span = std::max({mx[0] - mn[0], mx[1] - mn[1], 1e-6f});
    float scale = std::min(W, H) * 0.8f / span * m_zoom;
    float cx = (mx[0] + mn[0]) / 2, cy = (mx[1] + mn[1]) / 2;

    std::vector<float> zb((size_t)W * H, -1e30f);
    std::vector<QRgb> img((size_t)W * H, m_img.pixel(0, 0));

    auto put = [&](int x, int y, float z, QRgb col) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        size_t j = (size_t)y * W + x;
        if (z > zb[j]) {
            zb[j] = z;
            img[j] = col;
        }
    };

    for (const auto& f : m_faces) {
        if (f.verts.size() < 3) continue;
        const int nv = (int)f.verts.size();  // 3 or 4 (quads = single convex polygon)
        std::vector<std::array<float,3>> pts(nv);
        for (int j = 0; j < nv; ++j) {
            const auto& p = pv[f.verts[j]];
            pts[j] = {(p[0] - cx) * scale + W / 2.0f,
                      -(p[1] - cy) * scale + H / 2.0f, p[2]};
        }
        // flat normal (world space) for lambert fallback
        const auto& a = m_verts[f.verts[0]];
        const auto& b = m_verts[f.verts[1]];
        const auto& c = m_verts[f.verts[2]];
        float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
        float vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
        float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
        float ln = std::sqrt(nx * nx + ny * ny + nz * nz);
        float lam = 0.55f;
        if (ln > 1e-6f) {
            // two-sided: winding is unreliable for open meshes (levels)
            float d = std::fabs((nx * 0.4f + ny * 0.8f + nz * 0.45f) / ln);
            lam = std::max(0.3f, std::min(1.0f, d * 0.75f + 0.3f));
        }
        // stored per-corner normals (data, rotated into view space)
        float intens[4] = {lam, lam, lam, lam};
        if (f.hasNormals) {
            static const float lx = 0.4f / 1.14f, ly = 0.8f / 1.14f, lz = 0.45f / 1.14f;
            for (int j = 0; j < nv; ++j) {
                // apply the same rotX + yaw/pitch as vertices
                float ny = f.n[j][1] * crx - f.n[j][2] * srx;
                float nz2 = f.n[j][1] * srx + f.n[j][2] * crx;
                float x1 = f.n[j][0] * ca + nz2 * sa;
                float z1 = -f.n[j][0] * sa + nz2 * ca;
                float y2 = ny * cp - z1 * sp;
                float z2 = ny * sp + z1 * cp;
                float dot = std::fabs(x1 * lx + y2 * ly + z2 * lz);
                intens[j] = std::max(0.3f, std::min(1.0f, dot * 0.75f + 0.3f));
            }
        }
        // texture page origin in pixels (tpage: X in 64-halfword units, Y in 256 lines)
        int bppMode = (f.tpage >> 7) & 3;             // 0=4bpp, 1=8bpp, 2=16bpp
        int pxPerWord = bppMode == 0 ? 4 : bppMode == 1 ? 2 : 1;
        int pageX = (f.tpage & 0xF) * 64 * pxPerWord;
        int pageY = ((f.tpage >> 4) & 1) * 256;
        bool tex = f.textured && m_tex && f.tpage != 0 && !getenv("SB_NOTEX");

        // scanline fill of a convex 3/4-gon with linear UV + intensity
        // (matches the PS1 GPU's affine whole-quad mapping)
        float ymin = pts[0][1], ymax = pts[0][1];
        for (const auto& p : pts) { ymin = std::min(ymin, p[1]); ymax = std::max(ymax, p[1]); }
        for (int y = std::max(0, (int)ymin); y <= std::min(H - 1, (int)ymax); ++y) {
            struct Span { float x0, z0, u0, v0, i0, x1, z1, u1, v1, i1; };
            std::vector<Span> xs;
            for (int i = 0; i < nv; ++i) {
                const auto& pa = pts[i];
                const auto& pb = pts[(i + 1) % nv];
                if ((pa[1] <= y) != (pb[1] <= y)) {
                    float t = (y - pa[1]) / (pb[1] - pa[1] + 1e-12f);
                    float ua = f.uv[i][0], va = f.uv[i][1];
                    float ub = f.uv[(i + 1) % nv][0], vb = f.uv[(i + 1) % nv][1];
                    xs.push_back({pa[0] + t * (pb[0] - pa[0]),
                                  pa[2] + t * (pb[2] - pa[2]),
                                  ua + t * (ub - ua), va + t * (vb - va),
                                  intens[i] + t * (intens[(i + 1) % nv] - intens[i]),
                                  0, 0, 0, 0, 0});
                }
            }
            if (xs.size() != 2) continue;
            if (xs[0].x0 > xs[1].x0) std::swap(xs[0], xs[1]);
            int x0 = std::max(0, (int)xs[0].x0), x1 = std::min(W - 1, (int)xs[1].x0);
            for (int x = x0; x <= x1; ++x) {
                float t = (x - xs[0].x0) / (xs[1].x0 - xs[0].x0 + 1e-12f);
                float z = xs[0].z0 + t * (xs[1].z0 - xs[0].z0);
                float I = xs[0].i0 + t * (xs[1].i0 - xs[0].i0);
                if (!tex) {
                    put(x, y, z, qRgb(std::min(255, (int)(f.r * I)),
                                      std::min(255, (int)(f.g * I)),
                                      std::min(255, (int)(f.b * I))));
                } else {
                    float u = xs[0].u0 + t * (xs[1].u0 - xs[0].u0);
                    float v = xs[0].v0 + t * (xs[1].v0 - xs[0].v0);
                    uint8_t c4[4];
                    m_tex->sample(pageX + (int)u, pageY + (int)v, f.clut, c4);
                    if (c4[3] == 0) continue;  // transparent texel
                    // PS1 texture modulation: texel * color/128, then light
                    auto mod = [&](int i, uint8_t mc) -> int {
                        return std::min(255, (int)(c4[i] * (mc / 128.0f) * I));
                    };
                    put(x, y, z, qRgb(mod(0, f.r), mod(1, f.g), mod(2, f.b)));
                }
            }
        }
        if (getenv("SB_WIRE")) {  // wireframe overlay for geometry debugging
            for (int i = 0; i < nv; ++i) {
                const auto& pa = pts[i];
                const auto& pb = pts[(i + 1) % nv];
                float len = std::max(std::fabs(pb[0] - pa[0]), std::fabs(pb[1] - pa[1]));
                for (int s = 0; s <= (int)len; ++s) {
                    float t = len > 0 ? s / len : 0;
                    int x = (int)(pa[0] + t * (pb[0] - pa[0]));
                    int y = (int)(pa[1] + t * (pb[1] - pa[1]));
                    float z = pa[2] + t * (pb[2] - pa[2]);
                    put(x, y, z + 1e-3f, qRgb(0, 0, 0));
                }
            }
        }
    }
    m_img = QImage((const uchar*)img.data(), W, H, W * 4, QImage::Format_RGB32).copy();
    update();
}
