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
                         const std::vector<sb::QmdFace>& faces) {
    m_verts = verts;
    m_faces = faces;
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
    std::vector<std::array<float,3>> pv(m_verts.size());
    float mn[3] = {1e30f, 1e30f, 1e30f}, mx[3] = {-1e30f, -1e30f, -1e30f};
    for (size_t i = 0; i < m_verts.size(); ++i) {
        const auto& v = m_verts[i];
        float x1 = v.x * ca + v.z * sa;
        float z1 = -v.x * sa + v.z * ca;
        float y2 = v.y * cp - z1 * sp;
        float z2 = v.y * sp + z1 * cp;
        pv[i] = {x1, y2, z2};
        mn[0] = std::min(mn[0], x1); mx[0] = std::max(mx[0], x1);
        mn[1] = std::min(mn[1], y2); mx[1] = std::max(mx[1], y2);
        mn[2] = std::min(mn[2], z2); mx[2] = std::max(mx[2], z2);
    }
    float span = std::max({mx[0] - mn[0], mx[1] - mn[1], 1e-6f});
    float scale = std::min(W, H) * 0.8f / span * m_zoom;
    float cx = (mx[0] + mn[0]) / 2, cy = (mx[1] + mn[1]) / 2;

    std::vector<float> zb((size_t)W * H, -1e30f);

    auto put = [&](int x, int y, float z, QRgb col) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        size_t j = (size_t)y * W + x;
        if (z > zb[j]) {
            zb[j] = z;
            m_img.setPixel(x, y, col);
        }
    };

    for (const auto& f : m_faces) {
        if (f.verts.size() < 3) continue;
        // screen-space points (y flipped: world up -> screen up)
        std::vector<std::array<float,3>> pts;
        for (uint32_t vi : f.verts) {
            const auto& p = pv[vi];
            pts.push_back({(p[0] - cx) * scale + W / 2.0f,
                           -(p[1] - cy) * scale + H / 2.0f, p[2]});
        }
        // flat normal (world space) for lambert
        const auto& a = m_verts[f.verts[0]];
        const auto& b = m_verts[f.verts[1]];
        const auto& c = m_verts[f.verts[2]];
        float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
        float vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
        float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
        float ln = std::sqrt(nx * nx + ny * ny + nz * nz);
        float lam = 0.55f;
        if (ln > 1e-6f) {
            lam = std::max(0.15f, std::min(1.0f,
                (nx * 0.4f + ny * 0.8f + nz * 0.45f) / ln * 0.5f + 0.55f));
        }
        QRgb col = qRgb(std::min(255, (int)(f.r * lam)),
                        std::min(255, (int)(f.g * lam)),
                        std::min(255, (int)(f.b * lam)));

        // scanline fill (tris)
        float ymin = std::min({pts[0][1], pts[1][1], pts[2][1]});
        float ymax = std::max({pts[0][1], pts[1][1], pts[2][1]});
        for (int y = std::max(0, (int)ymin); y <= std::min(H - 1, (int)ymax); ++y) {
            std::vector<std::pair<float,float>> xs;  // (x, z)
            for (int i = 0; i < 3; ++i) {
                const auto& pa = pts[i];
                const auto& pb = pts[(i + 1) % 3];
                if ((pa[1] <= y) != (pb[1] <= y)) {
                    float t = (y - pa[1]) / (pb[1] - pa[1] + 1e-12f);
                    xs.push_back({pa[0] + t * (pb[0] - pa[0]), pa[2] + t * (pb[2] - pa[2])});
                }
            }
            if (xs.size() != 2) continue;
            if (xs[0].first > xs[1].first) std::swap(xs[0], xs[1]);
            for (int x = std::max(0, (int)xs[0].first); x <= std::min(W - 1, (int)xs[1].first); ++x) {
                float t = (x - xs[0].first) / (xs[1].first - xs[0].first + 1e-12f);
                put(x, y, xs[0].second + t * (xs[1].second - xs[0].second), col);
            }
        }
    }
    update();
}
