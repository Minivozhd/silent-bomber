#include "LabPanel.hpp"

#include "HexWidget.hpp"
#include "ModelView.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>

LabPanel::LabPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    m_info = new QLabel(this);
    m_info->setStyleSheet("font-weight: bold");
    lay->addWidget(m_info);

    auto* hlay = new QHBoxLayout();
    hlay->addWidget(new QLabel("Block:"));
    m_blockCombo = new QComboBox(this);
    m_blockCombo->setMinimumWidth(260);
    hlay->addWidget(m_blockCombo, 1);
    hlay->addWidget(new QLabel("Part:"));
    m_partCombo = new QComboBox(this);
    hlay->addWidget(m_partCombo, 1);
    lay->addLayout(hlay);

    auto* split = new QSplitter(this);
    lay->addWidget(split, 1);

    // left: parsing options
    auto* opts = new QWidget(split);
    auto* form = new QFormLayout(opts);
    const char* axisNames[3] = {"X", "Y", "Z"};
    for (int i = 0; i < 3; ++i) {
        auto* row = new QWidget(opts);
        auto* rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 0, 0, 0);
        auto* combo = new QComboBox(row);
        combo->addItems({"auto", "pair.a (x)", "pair.b", "pool.c"});
        rl->addWidget(combo, 1);
        auto* neg = new QCheckBox("negate", row);
        rl->addWidget(neg);
        m_axes[i] = {combo, neg};
        form->addRow(QString("axis %1:").arg(axisNames[i]), row);
    }
    m_quadPerm = new QComboBox(opts);
    m_quadPerm->addItems({"0,1,2,3 (GPU order)", "3,2,0,1", "2,0,1,3", "3,1,0,2", "2,3,1,0", "1,3,2,0"});
    form->addRow("quad corners:", m_quadPerm);
    m_triPerm = new QComboBox(opts);
    m_triPerm->addItems({"0,1,2", "0,2,1", "1,0,2", "1,2,0", "2,0,1", "2,1,0"});
    form->addRow("tri corners:", m_triPerm);
    m_uvSwap = new QCheckBox("swap u/v", opts);
    form->addRow("uv:", m_uvSwap);
    m_uvMirrorU = new QCheckBox("mirror u", opts);
    form->addRow("", m_uvMirrorU);
    m_uvMirrorV = new QCheckBox("mirror v", opts);
    form->addRow("", m_uvMirrorV);
    m_storedNormals = new QCheckBox("stored normals (tails/pools)", opts);
    m_storedNormals->setChecked(true);
    form->addRow("shading:", m_storedNormals);
    m_textures = new QCheckBox("textures", opts);
    m_textures->setChecked(true);
    form->addRow("render:", m_textures);
    m_wire = new QCheckBox("wireframe overlay", opts);
    form->addRow("", m_wire);
    m_baked = new QCheckBox("baked vertex colors (no relight)", opts);
    form->addRow("", m_baked);
    form->addRow(new QLabel("Reparse is live: change anything\nand the model re-renders.", opts));
    split->addWidget(opts);

    // center: model
    m_model = new ModelView(split);
    split->addWidget(m_model);

    // right: hex
    m_hex = new HexWidget(split);
    m_hex->setMinimumWidth(620);
    split->addWidget(m_hex);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setStretchFactor(2, 0);

    auto changed = [this]() { reparse(); };
    for (auto& a : m_axes) {
        connect(a.src, &QComboBox::currentIndexChanged, this, changed);
        connect(a.neg, &QCheckBox::toggled, this, changed);
    }
    connect(m_quadPerm, &QComboBox::currentIndexChanged, this, changed);
    connect(m_triPerm, &QComboBox::currentIndexChanged, this, changed);
    connect(m_uvSwap, &QCheckBox::toggled, this, changed);
    connect(m_uvMirrorU, &QCheckBox::toggled, this, changed);
    connect(m_uvMirrorV, &QCheckBox::toggled, this, changed);
    connect(m_storedNormals, &QCheckBox::toggled, this, changed);
    connect(m_textures, &QCheckBox::toggled, this, [this](bool on) { m_model->setTextures(on); });
    connect(m_wire, &QCheckBox::toggled, this, [this](bool on) { m_model->setWireframe(on); });
    connect(m_baked, &QCheckBox::toggled, this, [this](bool on) { m_model->setBaked(on); });
    connect(m_blockCombo, &QComboBox::currentIndexChanged, this, [this](int) { updateBlockView(); });
    connect(m_partCombo, &QComboBox::currentIndexChanged, this, [this](int) { updateBlockView(); });
}

void LabPanel::loadFile(const QString& path, const QString& preselectBlock) {
    m_path = path;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QByteArray raw = f.readAll();
    m_bytes.assign(raw.begin(), raw.end());

    // textures from the sibling part1 bundle
    QString p1 = path;
    p1.replace(".part3_raw.bin", ".part1_rle.bin");
    QFile f1(p1);
    if (f1.open(QIODevice::ReadOnly)) {
        QByteArray raw1 = f1.readAll();
        m_tex.load((const uint8_t*)raw1.constData(), raw1.size());
    } else {
        m_tex.tims.clear();
    }
    reparse();

    if (!preselectBlock.isEmpty()) {
        int idx = m_blockCombo->findText(preselectBlock, Qt::MatchStartsWith);
        if (idx >= 0) m_blockCombo->setCurrentIndex(idx);
        // known-good axis mapping for the CMFANR00 character face:
        // x = pool.c (lateral), y = -pair.b (up), z = pair.a (nose fwd)
        if (preselectBlock.startsWith("CMFANR")) {
            m_axes[0].src->setCurrentIndex(3);  // pool.c
            m_axes[1].src->setCurrentIndex(2);  // pair.b
            m_axes[1].neg->setChecked(true);
            m_axes[2].src->setCurrentIndex(1);  // pair.a
        }
    }
}

void LabPanel::reparse() {
    // options -> global parse opts
    for (int i = 0; i < 3; ++i) {
        sb::g_parseOpts.axisSrc[i] = m_axes[i].src->currentIndex() - 1;  // 0 = auto
        sb::g_parseOpts.axisSgn[i] = m_axes[i].neg->isChecked() ? -1 : 1;
    }
    sb::g_parseOpts.quadPerm = m_quadPerm->currentIndex();
    sb::g_parseOpts.triPerm = m_triPerm->currentIndex();
    sb::g_parseOpts.uvFlip = (m_uvSwap->isChecked() ? 1 : 0) |
                             (m_uvMirrorU->isChecked() ? 2 : 0) |
                             (m_uvMirrorV->isChecked() ? 4 : 0);
    sb::g_parseOpts.storedNormals = m_storedNormals->isChecked();
    sb::g_parseOpts.onlyPart = -1;  // parts via the combo instead

    m_blocks = sb::parseQmdContainer(m_bytes.data(), m_bytes.size());

    m_blockCombo->blockSignals(true);
    QString prev = m_blockCombo->currentText();
    m_blockCombo->clear();
    for (const auto& b : m_blocks) {
        QString label = QString::fromStdString(b.name) +
                        QString(" — %1v %2f%3")
                            .arg(b.verts.size()).arg(b.faces.size())
                            .arg(b.simple ? "" : " (complex)");
        m_blockCombo->addItem(label);
    }
    int idx = m_blockCombo->findText(prev);
    m_blockCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    m_blockCombo->blockSignals(false);
    updateBlockView();
}

void LabPanel::updateBlockView() {
    int row = m_blockCombo->currentIndex();
    if (row < 0 || row >= (int)m_blocks.size()) return;
    const auto& b = m_blocks[row];

    m_partCombo->blockSignals(true);
    m_partCombo->clear();
    if (!b.parts.empty()) {
        m_partCombo->addItem(QString("all %1 parts (raw)").arg(b.parts.size()));
        for (size_t i = 0; i < b.parts.size(); ++i)
            m_partCombo->addItem(QString("part %1 — %2v %3f").arg(i).arg(b.parts[i].verts.size()).arg(b.parts[i].faces.size()));
    }
    m_partCombo->setVisible(!b.parts.empty());
    m_partCombo->blockSignals(false);

    int part = m_partCombo->currentIndex();
    const sb::QmdBlock* show = &b;
    if (part > 0 && part <= (int)b.parts.size()) show = &b.parts[part - 1];
    m_model->setModel(show->verts, show->faces, &m_tex);

    // hex: the block's raw bytes with region highlights
    m_hex->setData(m_bytes.data(), m_bytes.size());
    std::vector<std::tuple<size_t, size_t, QString>> regs;
    for (const auto& [r0, r1, label] : b.regions)
        regs.emplace_back(r0, r1, QString::fromStdString(label));
    m_hex->setRegions(std::move(regs));
    m_hex->setBase(b.offset);

    m_info->setText(QString("%1 — block %2 @ 0x%3, %4 verts, %5 faces, %6 TIMs in bank")
                        .arg(QFileInfo(m_path).fileName())
                        .arg(QString::fromStdString(b.name))
                        .arg(b.offset, 0, 16)
                        .arg(b.verts.size()).arg(b.faces.size()).arg(m_tex.tims.size()));
}
