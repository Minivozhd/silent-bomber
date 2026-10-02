#include "HexWidget.hpp"

#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

HexWidget::HexWidget(QWidget* parent) : QWidget(parent) {
    setFont(QFont("Menlo", 11));
    QFontMetricsF fm(font());
    m_charW = fm.horizontalAdvance('0');
    m_charH = fm.height();
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(160);
}

void HexWidget::setData(const uint8_t* data, size_t size) {
    m_data = data;
    m_size = size;
    update();
}

void HexWidget::setRegions(std::vector<std::tuple<size_t, size_t, QString>> regions) {
    m_regions = std::move(regions);
    update();
}

void HexWidget::setBase(size_t off) {
    m_base = off & ~0xFUL;
    update();
}

void HexWidget::setSelection(size_t off, size_t len) {
    m_selOff = off;
    m_selLen = len;
    if (len) setBase(off > 32 ? off - 32 : 0);
    update();
}

void HexWidget::wheelEvent(QWheelEvent* e) {
    int rows = (height() - (m_charH + 2)) / m_charH;
    int64_t nb = (int64_t)m_base + (int64_t)e->angleDelta().y() / 120 * -16 * 3;
    int64_t maxb = (int64_t)m_size - (int64_t)rows * 16;
    nb = std::max<int64_t>(0, std::min(nb, std::max<int64_t>(0, maxb)));
    m_base = (size_t)nb & ~0xFLL;
    update();
}

void HexWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(28, 28, 34));
    if (!m_data) return;

    const QString hex = "0123456789ABCDEF";
    const int rulerH = m_charH + 2;
    const int offW = m_charW * 10;  // left offset ruler width

    // column ruler
    p.setPen(QColor(150, 150, 160));
    for (int c = 0; c < 16; ++c)
        p.drawText(offW + c * (m_charW * 3), 0, m_charW * 2, rulerH, 0, QString::number(c, 16).toUpper().rightJustified(2, '0'));

    int rows = (height() - rulerH) / m_charH;
    size_t start = m_base;
    size_t end = std::min(m_size, start + (size_t)rows * 16);

    static const QColor palette[] = {
        QColor(70, 100, 70),   // greenish
        QColor(100, 80, 50),   // orange-ish
        QColor(60, 80, 110),   // blue-ish
        QColor(95, 70, 110),   // purple-ish
        QColor(110, 70, 70),   // red-ish
        QColor(70, 100, 100),  // teal
    };

    for (int row = 0; row < rows; ++row) {
        size_t rowOff = start + (size_t)row * 16;
        if (rowOff >= end) break;
        int y = rulerH + row * m_charH;
        // offset ruler
        p.setPen(QColor(150, 150, 160));
        p.drawText(0, y, offW, m_charH, 0, QString("%1:").arg(rowOff, 8, 16, QLatin1Char('0')).toUpper());
        for (int c = 0; c < 16 && rowOff + c < end; ++c) {
            size_t off = rowOff + c;
            // region background
            int ri = 0;
            for (const auto& [r0, r1, label] : m_regions) {
                if (off >= r0 && off < r1) {
                    p.fillRect(offW + c * (m_charW * 3), y, m_charW * 3, m_charH,
                               palette[ri % 6]);
                    break;
                }
                ++ri;
            }
            if (m_selLen && off >= m_selOff && off < m_selOff + m_selLen)
                p.fillRect(offW + c * (m_charW * 3), y, m_charW * 3, m_charH, QColor(160, 60, 60));
            p.setPen(QColor(220, 220, 225));
            uint8_t b = m_data[off];
            QString s;
            s += hex[b >> 4];
            s += hex[b & 15];
            p.drawText(offW + c * (m_charW * 3), y, m_charW * 3, m_charH, 0, s);
        }
    }
}
