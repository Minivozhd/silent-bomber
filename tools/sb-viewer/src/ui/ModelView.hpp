#pragma once

#include <QWidget>
#include <QImage>
#include <vector>

#include "../formats/QmdModel.hpp"
#include "../formats/TextureBank.hpp"

/// Software-rasterized model view (PS1 polycounts are tiny; no GL needed —
/// macOS 26 has no AGL and we avoid the QtOpenGL dependency entirely).
/// Left-drag orbits, wheel zooms.
class ModelView : public QWidget {
    Q_OBJECT
public:
    explicit ModelView(QWidget* parent = nullptr);

    void setModel(const std::vector<sb::QmdVertex>& verts,
                  const std::vector<sb::QmdFace>& faces,
                  const sb::TextureBank* tex = nullptr);
    void clear();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    void render();

    std::vector<sb::QmdVertex> m_verts;
    std::vector<sb::QmdFace> m_faces;
    const sb::TextureBank* m_tex = nullptr;
    QImage m_img;
    float m_yaw = 0.6f, m_pitch = -0.4f, m_zoom = 1.0f;
    void applyEnvCam();  // SB_YAW/SB_PITCH/SB_ZOOM overrides (headless shots)
    QPoint m_lastPos;
    bool m_dragging = false;
};
