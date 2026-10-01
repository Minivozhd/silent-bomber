#include "ViewportPanel.hpp"
#include "ModelView.hpp"

#include <QComboBox>
#include <QFileInfo>
#include <QImageReader>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QVBoxLayout>

#include "../formats/QmdModel.hpp"

// Simple image display with checkerboard background for alpha.
class ImageView : public QWidget {
public:
    explicit ImageView(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumSize(160, 120);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    void setImage(const QImage& img) {
        m_img = img;
        update();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        // checkerboard
        const int s = 8;
        for (int y = 0; y < height(); y += s)
            for (int x = 0; x < width(); x += s)
                p.fillRect(x, y, s, s, ((x / s + y / s) % 2) ? QColor(90, 90, 90) : QColor(60, 60, 60));
        if (m_img.isNull()) return;
        p.drawImage(0, 0, m_img);
    }
private:
    QImage m_img;
};

ViewportPanel::ViewportPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    m_info = new QLabel(this);
    m_info->setStyleSheet("font-weight: bold");
    lay->addWidget(m_info);

    m_split = new QSplitter(this);
    m_list = new QListWidget(m_split);
    m_list->setMaximumWidth(260);
    auto* scroll = new QScrollArea(m_split);
    m_scroll = scroll;
    auto* view = new ImageView(scroll);
    scroll->setWidget(view);
    scroll->setWidgetResizable(false);
    m_image = view;
    // right side: model view with a part selector on top (complex blocks)
    auto* modelBox = new QWidget(m_split);
    m_modelBox = modelBox;
    auto* modelLay = new QVBoxLayout(modelBox);
    modelLay->setContentsMargins(0, 0, 0, 0);
    m_partCombo = new QComboBox(modelBox);
    m_partCombo->setVisible(false);
    modelLay->addWidget(m_partCombo);
    m_model = new ModelView(modelBox);
    modelLay->addWidget(m_model);
    modelBox->setVisible(false);
    m_split->addWidget(m_list);
    m_split->addWidget(scroll);
    m_split->addWidget(modelBox);
    m_split->setStretchFactor(1, 1);
    m_split->setStretchFactor(2, 1);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row < 0) return;
        if (m_modelBox->isVisible()) {
            if (row >= (int)m_blocks.size()) return;
            const auto& b = m_blocks[row];
            if (b.parts.empty()) {
                m_partCombo->setVisible(false);
                m_model->setModel(b.verts, b.faces, &m_tex);
            } else {
                m_partCombo->blockSignals(true);
                m_partCombo->clear();
                m_partCombo->addItem(QString("all %1 parts (assembled)").arg(b.parts.size()));
                for (size_t i = 0; i < b.parts.size(); ++i)
                    m_partCombo->addItem(QString("part %1 — %2 verts, %3 faces")
                                             .arg(i).arg(b.parts[i].verts.size()).arg(b.parts[i].faces.size()));
                m_partCombo->setCurrentIndex(0);
                m_partCombo->blockSignals(false);
                m_partCombo->setVisible(true);
                m_model->setModel(b.verts, b.faces, &m_tex);
            }
            return;
        }
        if (row >= (int)m_tims.size()) return;
        std::vector<uint8_t> rgba;
        sb::timToRgba(m_tims[row], rgba);
        QImage img(rgba.data(), m_tims[row].width, m_tims[row].height, QImage::Format_RGBA8888);
        m_image->setImage(img.copy());
        m_image->resize(m_tims[row].width, m_tims[row].height);
    });

    connect(m_partCombo, &QComboBox::currentIndexChanged, this, [this](int idx) {
        int row = m_list->currentRow();
        if (row < 0 || row >= (int)m_blocks.size()) return;
        const auto& b = m_blocks[row];
        if (idx <= 0 || idx > (int)b.parts.size())
            m_model->setModel(b.verts, b.faces, &m_tex);
        else
            m_model->setModel(b.parts[idx - 1].verts, b.parts[idx - 1].faces, &m_tex);
    });
}

void ViewportPanel::showFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QByteArray raw = f.readAll();
    m_bytes.assign(raw.begin(), raw.end());

    if (path.contains(".part1")) {
        showTimBundle(path);
    } else if (path.contains(".part3")) {
        showQmdContainer(path);
    } else {
        m_list->clear();
        m_tims.clear();
        m_blocks.clear();
        m_modelBox->setVisible(false);
        m_partCombo->setVisible(false);
        m_scroll->setVisible(true);
        m_info->setText(QFileInfo(path).fileName() +
                        QString(" — %1 bytes (mission-data viewer pending format research)").arg(raw.size()));
    }
}

void ViewportPanel::showTimBundle(const QString& path) {
    m_tims.clear();
    m_blocks.clear();
    m_modelBox->setVisible(false);
    m_partCombo->setVisible(false);
    m_scroll->setVisible(true);
    m_image->setVisible(true);
    m_list->clear();
    auto offs = sb::findTims(m_bytes.data(), m_bytes.size());
    for (size_t o : offs) {
        sb::TimImage t;
        if (sb::parseTim(m_bytes.data() + o, m_bytes.size() - o, t))
            m_tims.push_back(std::move(t));
    }
    for (size_t i = 0; i < m_tims.size(); ++i) {
        const auto& t = m_tims[i];
        m_list->addItem(QString("TIM %1 — %2x%3 %4bpp")
                            .arg(i)
                            .arg(t.width)
                            .arg(t.height)
                            .arg(t.bpp));
    }
    m_info->setText(QString("%1 — %2 TIM images").arg(QFileInfo(path).fileName()).arg(m_tims.size()));
    if (!m_tims.empty()) m_list->setCurrentRow(0);
}

void ViewportPanel::showQmdContainer(const QString& path) {
    m_tims.clear();
    m_model->clear();
    m_list->clear();
    m_blocks = sb::parseQmdContainer(m_bytes.data(), m_bytes.size());
    // textures: the sibling part1 TIM bundle of the same package
    QString p1 = path;
    p1.replace(".part3_raw.bin", ".part1_rle.bin");
    QFile f1(p1);
    if (f1.open(QIODevice::ReadOnly)) {
        QByteArray raw1 = f1.readAll();
        m_tex.load((const uint8_t*)raw1.constData(), raw1.size());
    } else {
        m_tex.tims.clear();
    }
    for (const auto& b : m_blocks) {
        QString label = QString::fromStdString(b.name);
        if (b.simple)
            label += QString(" — %1 verts, %2 faces").arg(b.verts.size()).arg(b.faces.size());
        else
            label += QString(" — %1 verts, %2 faces (complex)").arg(b.verts.size()).arg(b.faces.size());
        m_list->addItem(label);
    }
    m_info->setText(QString("%1 — %2 QMD blocks, %3 TIMs in texture bank")
                        .arg(QFileInfo(path).fileName())
                        .arg(m_blocks.size())
                        .arg(m_tex.tims.size()));
    m_scroll->setVisible(false);
    m_image->setVisible(false);
    m_modelBox->setVisible(true);
    for (size_t i = 0; i < m_blocks.size(); ++i) {
        if (!m_blocks[i].verts.empty()) {
            m_list->setCurrentRow((int)i);
            break;
        }
    }
}
