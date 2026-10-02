#include "UvView.hpp"

#include <QPainter>
#include <QMouseEvent>

UvView::UvView(QWidget* parent) : QWidget(parent) {
    setMinimumSize(260, 260);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void UvView::setModel(const std::vector<sb::QmdFace>& faces, const sb::TextureBank* tex) {
    m_faces = faces;
    m_tex = tex;
    m_page = QImage();
    m_selected = -1;

    // render the texture page of the first textured face (most blocks use one)
    for (const auto& f : m_faces) {
        if (!f.textured || !m_tex) continue;
        int bppMode = (f.tpage >> 7) & 3;
        int pxPerWord = bppMode == 0 ? 4 : bppMode == 1 ? 2 : 1;
        m_pageX = (f.tpage & 0xF) * 64 * pxPerWord;
        m_pageY = ((f.tpage >> 4) & 1) * 256;
        m_page = QImage(256, 256, QImage::Format_RGB32);
        for (int y = 0; y < 256; ++y)
            for (int x = 0; x < 256; ++x) {
                uint8_t c[4];
                m_tex->sample(m_pageX + x, m_pageY + y, f.clut, c);
                m_page.setPixel(x, y, c[3] ? qRgb(c[0], c[1], c[2]) : qRgb(30, 30, 38));
            }
        break;
    }
    update();
}

void UvView::setSelected(int face) {
    m_selected = face;
    update();
}

void UvView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(24, 24, 32));
    if (m_page.isNull()) {
        p.setPen(QColor(150, 150, 160));
        p.drawText(rect(), Qt::AlignCenter, "no textured faces");
        return;
    }
    int side = std::min(width(), height());
    p.drawImage(QRect(0, 0, side, side), m_page);
    float k = side / 256.0f;
    // UV polygons
    for (size_t fi = 0; fi < m_faces.size(); ++fi) {
        const auto& f = m_faces[fi];
        if (!f.textured) continue;
        bool sel = (int)fi == m_selected;
        p.setPen(QPen(sel ? QColor(255, 80, 80) : QColor(90, 220, 255), sel ? 2 : 1));
        int n = (int)f.verts.size();
        for (int j = 0; j < n; ++j) {
            int j2 = (j + 1) % n;
            p.drawLine(QPointF(f.uv[j][0] * k, f.uv[j][1] * k),
                       QPointF(f.uv[j2][0] * k, f.uv[j2][1] * k));
        }
    }
}

void UvView::mousePressEvent(QMouseEvent* e) {
    int side = std::min(width(), height());
    float k = side / 256.0f;
    float mx = e->pos().x() / k, my = e->pos().y() / k;
    // nearest UV centroid
    int best = -1;
    float bestD = 1e9f;
    for (size_t fi = 0; fi < m_faces.size(); ++fi) {
        const auto& f = m_faces[fi];
        if (!f.textured) continue;
        float cx = 0, cy = 0;
        for (size_t j = 0; j < f.verts.size(); ++j) { cx += f.uv[j][0]; cy += f.uv[j][1]; }
        cx /= f.verts.size(); cy /= f.verts.size();
        float d = (cx - mx) * (cx - mx) + (cy - my) * (cy - my);
        if (d < bestD) { bestD = d; best = (int)fi; }
    }
    if (best >= 0) {
        m_selected = best;
        emit faceSelected(best);
        update();
    }
}
