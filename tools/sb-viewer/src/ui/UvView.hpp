#pragma once

#include <QWidget>
#include <vector>

#include "../formats/QmdModel.hpp"
#include "../formats/TextureBank.hpp"

/// UV-space view: the texture page used by the block with the faces' UV
/// polygons overlaid. Click near a polygon to select that face.
class UvView : public QWidget {
    Q_OBJECT
public:
    explicit UvView(QWidget* parent = nullptr);

    void setModel(const std::vector<sb::QmdFace>& faces, const sb::TextureBank* tex);
    void setSelected(int face);

signals:
    void faceSelected(int idx);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

private:
    std::vector<sb::QmdFace> m_faces;
    const sb::TextureBank* m_tex = nullptr;
    QImage m_page;
    int m_pageX = 0, m_pageY = 0;  // pixel origin of the rendered page
    int m_selected = -1;
};
