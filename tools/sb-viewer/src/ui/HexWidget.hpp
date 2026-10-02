#pragma once

#include <QWidget>
#include <vector>

/// Hex dump with row (file offset) and column (00..0F) rulers and
/// region background highlights.
class HexWidget : public QWidget {
public:
    explicit HexWidget(QWidget* parent = nullptr);

    void setData(const uint8_t* data, size_t size);
    void setRegions(std::vector<std::tuple<size_t, size_t, QString>> regions);
    void setBase(size_t off);  // first displayed offset

protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    const uint8_t* m_data = nullptr;
    size_t m_size = 0;
    size_t m_base = 0;
    std::vector<std::tuple<size_t, size_t, QString>> m_regions;
    int m_charW = 8, m_charH = 14;
};
