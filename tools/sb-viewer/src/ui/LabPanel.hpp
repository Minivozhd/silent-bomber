#pragma once

#include <QWidget>
#include <vector>

#include "../formats/QmdModel.hpp"
#include "../formats/TextureBank.hpp"

class QComboBox;
class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QTableWidget;
class ModelView;
class HexWidget;
class UvView;

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
    void showRecord();
    void loadSpecEditor();

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
    QCheckBox* m_normals = nullptr;
    QCheckBox* m_simpleStoredNormals = nullptr;
    QComboBox* m_nrmSource = nullptr;
    QComboBox* m_uvLayer = nullptr;
    QSpinBox* m_nrmIdxOff = nullptr;
    QComboBox* m_nrmByteSel = nullptr;
    QCheckBox* m_smoothGeo = nullptr;
    QComboBox* m_nrmPerm = nullptr;
    QCheckBox* m_nrmSgn[3] = {nullptr, nullptr, nullptr};
    QSpinBox* m_idxShift = nullptr;
    UvView* m_uv = nullptr;
    QComboBox* m_recType = nullptr;
    QLineEdit* m_specEdit = nullptr;
    QLabel* m_specInfo = nullptr;
    QSpinBox* m_recSizeOv = nullptr;
    QSpinBox* m_recSpin = nullptr;
    QTableWidget* m_recTable = nullptr;
};
