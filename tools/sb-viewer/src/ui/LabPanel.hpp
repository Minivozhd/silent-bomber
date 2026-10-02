#pragma once

#include <QWidget>
#include <vector>

#include "../formats/QmdModel.hpp"
#include "../formats/TextureBank.hpp"

class QComboBox;
class QCheckBox;
class QLabel;
class ModelView;
class HexWidget;

/// Interactive model-format lab: a QMD block viewer with live parsing
/// parameters (axis mapping, corner orders, UV transforms, normals) and a
/// hex view of the block with structural region highlighting.
class LabPanel : public QWidget {
    Q_OBJECT
public:
    explicit LabPanel(QWidget* parent = nullptr);

    void loadFile(const QString& path, const QString& preselectBlock = QString());

private:
    void reparse();
    void updateBlockView();

    QString m_path;
    std::vector<uint8_t> m_bytes;
    std::vector<sb::QmdBlock> m_blocks;
    sb::TextureBank m_tex;

    QComboBox* m_blockCombo = nullptr;
    QComboBox* m_partCombo = nullptr;
    QLabel* m_info = nullptr;
    ModelView* m_model = nullptr;
    HexWidget* m_hex = nullptr;

    struct AxisRow { QComboBox* src; QCheckBox* neg; };
    AxisRow m_axes[3];
    QComboBox* m_quadPerm = nullptr;
    QComboBox* m_triPerm = nullptr;
    QCheckBox* m_uvSwap = nullptr;
    QCheckBox* m_uvMirrorU = nullptr;
    QCheckBox* m_uvMirrorV = nullptr;
    QCheckBox* m_storedNormals = nullptr;
    QCheckBox* m_textures = nullptr;
    QCheckBox* m_wire = nullptr;
    QCheckBox* m_baked = nullptr;
};
