#pragma once

#include <QLabel>
#include <QListWidget>
#include <QSplitter>
#include <QWidget>
#include <vector>

#include "../formats/TimImage.hpp"

class ImageView;

/// Viewport: shows a package part file. part1 bundles -> TIM gallery;
/// other parts -> hex/info stub (model/level viewers land here later).
class ViewportPanel : public QWidget {
    Q_OBJECT
public:
    explicit ViewportPanel(QWidget* parent = nullptr);

    void showFile(const QString& path);

private:
    void showTimBundle(const QString& path);

    QListWidget* m_list = nullptr;
    ImageView* m_image = nullptr;
    QLabel* m_info = nullptr;
    QSplitter* m_split = nullptr;

    std::vector<sb::TimImage> m_tims;
    std::vector<uint8_t> m_bytes;
};
