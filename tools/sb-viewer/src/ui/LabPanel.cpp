#include "LabPanel.hpp"

#include "HexWidget.hpp"
#include "ModelView.hpp"
#include "UvView.hpp"
#include "../formats/RecSpec.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

LabPanel::LabPanel(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    m_info = new QLabel(this);
    m_info->setStyleSheet("font-weight: bold");
    lay->addWidget(m_info);

    auto* hlay = new QHBoxLayout();
    hlay->addWidget(new QLabel("Block:"));
    m_blockCombo = new QComboBox(this);
    m_blockCombo->setMinimumWidth(220);
    hlay->addWidget(m_blockCombo, 1);
    hlay->addWidget(new QLabel("Part:"));
    m_partCombo = new QComboBox(this);
    hlay->addWidget(m_partCombo, 1);
    lay->addLayout(hlay);

    auto* split = new QSplitter(this);
    lay->addWidget(split, 1);

    // ---- left: options + record format editor (scrollable)
    auto* optsScroll = new QScrollArea(split);
    optsScroll->setWidgetResizable(true);
    auto* opts = new QWidget();
    optsScroll->setWidget(opts);
    auto* form = new QFormLayout(opts);
    const char* axisNames[3] = {"X", "Y", "Z"};
    for (int i = 0; i < 3; ++i) {
        auto* row = new QWidget(opts);
        auto* rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 0, 0, 0);
        auto* combo = new QComboBox(row);
        combo->addItems({"auto", "pair.a (x)", "pair.b", "pool.c"});
        rl->addWidget(combo, 1);
        auto* neg = new QCheckBox("neg", row);
        rl->addWidget(neg);
        m_axes[i] = {combo, neg};
        form->addRow(QString("axis %1:").arg(axisNames[i]), row);
    }
    m_quadPerm = new QComboBox(opts);
    for (const char* q : {
            "0,1,2,3 (GPU order)", "0,1,3,2", "0,2,1,3", "0,2,3,1", "0,3,1,2", "0,3,2,1",
            "1,0,2,3", "1,0,3,2", "1,2,0,3", "1,2,3,0", "1,3,0,2", "1,3,2,0",
            "2,0,1,3", "2,0,3,1", "2,1,0,3", "2,1,3,0", "2,3,0,1", "2,3,1,0",
            "3,0,1,2", "3,0,2,1", "3,1,0,2", "3,1,2,0", "3,2,0,1", "3,2,1,0 (reversed)"})
        m_quadPerm->addItem(q);
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
    m_idxShift = new QSpinBox(opts);
    m_idxShift->setRange(0, 3);
    m_idxShift->setValue(1);
    form->addRow("index shift (x2):", m_idxShift);
    m_storedNormals = new QCheckBox("stored normals (tails/pools)", opts);
    m_storedNormals->setChecked(true);
    form->addRow("shading:", m_storedNormals);
    m_textures = new QCheckBox("textures", opts);
    m_textures->setChecked(true);
    form->addRow("render:", m_textures);
    m_wire = new QCheckBox("wireframe overlay", opts);
    form->addRow("", m_wire);
    m_normals = new QCheckBox("show normals", opts);
    form->addRow("", m_normals);
    m_baked = new QCheckBox("baked vertex colors (no relight)", opts);
    form->addRow("", m_baked);

    // ---- record format editor
    form->addRow(new QLabel("—— record format (struct-style) ——", opts));
    m_recType = new QComboBox(opts);
    for (int t : {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11})
        m_recType->addItem(QString("type %1").arg(t), t);
    form->addRow("type:", m_recType);
    m_specEdit = new QLineEdit(opts);
    m_specEdit->setFont(QFont("Menlo", 10));
    m_specEdit->setPlaceholderText("v0:H v1:H u0:B w0:B cba:H rgb:I ...");
    form->addRow("layout:", m_specEdit);
    m_specInfo = new QLabel(opts);
    m_specInfo->setWordWrap(true);
    form->addRow(m_specInfo);
    m_recSizeOv = new QSpinBox(opts);
    m_recSizeOv->setRange(0, 128);
    m_recSizeOv->setValue(0);
    m_recSizeOv->setSpecialValueText("auto");
    form->addRow("rec size:", m_recSizeOv);
    auto* specBtns = new QWidget(opts);
    auto* sbl = new QHBoxLayout(specBtns);
    sbl->setContentsMargins(0, 0, 0, 0);
    auto* applyBtn = new QPushButton("apply", specBtns);
    auto* resetBtn = new QPushButton("reset", specBtns);
    sbl->addWidget(applyBtn);
    sbl->addWidget(resetBtn);
    form->addRow(specBtns);
    split->addWidget(optsScroll);

    // ---- center: model
    m_model = new ModelView(split);
    split->addWidget(m_model);

    // ---- right: uv view + record inspector + hex
    auto* right = new QWidget(split);
    auto* rl2 = new QVBoxLayout(right);
    rl2->setContentsMargins(0, 0, 0, 0);
    auto* rsplit = new QSplitter(Qt::Vertical, right);
    m_uv = new UvView(rsplit);
    rsplit->addWidget(m_uv);
    auto* inspBox = new QWidget(rsplit);
    auto* il = new QVBoxLayout(inspBox);
    il->setContentsMargins(0, 0, 0, 0);
    auto* irl = new QHBoxLayout();
    irl->addWidget(new QLabel("record:"));
    m_recSpin = new QSpinBox(inspBox);
    m_recSpin->setRange(0, 0);
    irl->addWidget(m_recSpin);
    irl->addStretch(1);
    il->addLayout(irl);
    m_recTable = new QTableWidget(0, 3, inspBox);
    m_recTable->setHorizontalHeaderLabels({"field", "value", "raw"});
    m_recTable->setMaximumHeight(150);
    il->addWidget(m_recTable);
    rsplit->addWidget(inspBox);
    m_hex = new HexWidget(rsplit);
    m_hex->setMinimumHeight(200);
    rsplit->addWidget(m_hex);
    rsplit->setStretchFactor(0, 1);
    rsplit->setStretchFactor(2, 1);
    rl2->addWidget(rsplit);
    split->addWidget(right);
    split->setStretchFactor(0, 0);
    split->setStretchFactor(1, 1);
    split->setStretchFactor(2, 1);

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
    connect(m_idxShift, &QSpinBox::valueChanged, this, changed);
    connect(m_storedNormals, &QCheckBox::toggled, this, changed);
    connect(m_textures, &QCheckBox::toggled, this, [this](bool on) { m_model->setTextures(on); });
    connect(m_wire, &QCheckBox::toggled, this, [this](bool on) { m_model->setWireframe(on); });
    connect(m_normals, &QCheckBox::toggled, this, [this](bool on) { m_model->setNormals(on); });
    connect(m_baked, &QCheckBox::toggled, this, [this](bool on) { m_model->setBaked(on); });
    connect(m_blockCombo, &QComboBox::currentIndexChanged, this, [this](int) { updateBlockView(); });
    connect(m_partCombo, &QComboBox::currentIndexChanged, this, [this](int) { updateBlockView(); });
    connect(m_recSpin, &QSpinBox::valueChanged, this, [this](int) { showRecord(); });

    connect(applyBtn, &QPushButton::clicked, this, [this]() {
        int type = m_recType->currentData().toInt();
        QString text = m_specEdit->text().trimmed();
        if (text.isEmpty()) {
            sb::g_parseOpts.recSpecs.erase(type);
        } else {
            auto spec = sb::parseRecSpec(text.toStdString());
            if (!spec.valid) {
                m_specInfo->setText("error: " + QString::fromStdString(spec.error));
                m_specInfo->setStyleSheet("color: #e66");
                return;
            }
            sb::g_parseOpts.recSpecs[type] = text.toStdString();
        }
        int ov = m_recSizeOv->value();
        if (ov > 0) sb::g_parseOpts.recSizeOverride[type] = ov;
        else sb::g_parseOpts.recSizeOverride.erase(type);
        reparse();
    });
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        sb::g_parseOpts.recSpecs.clear();
        sb::g_parseOpts.recSizeOverride.clear();
        m_recSizeOv->setValue(0);
        loadSpecEditor();
        reparse();
    });
    connect(m_recType, &QComboBox::currentIndexChanged, this, [this](int) { loadSpecEditor(); });
    connect(m_uv, &UvView::faceSelected, this, [this](int idx) {
        m_recSpin->setValue(idx);
    });
    loadSpecEditor();
}

void LabPanel::loadSpecEditor() {
    int type = m_recType->currentData().toInt();
    auto it = sb::g_parseOpts.recSpecs.find(type);
    std::string text = (it != sb::g_parseOpts.recSpecs.end())
                           ? it->second
                           : sb::defaultRecSpecs().at(type);
    m_specEdit->setText(QString::fromStdString(text));
    auto spec = sb::parseRecSpec(text);
    m_specInfo->setStyleSheet("");
    m_specInfo->setText(spec.valid ? QString("computed record size: %1 bytes").arg(spec.size)
                                   : "error: " + QString::fromStdString(spec.error));
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
    sb::g_parseOpts.idxShift = m_idxShift->value();
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
    m_uv->setModel(show->faces, &m_tex);

    m_recSpin->setRange(0, std::max(0, (int)show->faces.size() - 1));
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
    showRecord();
}

void LabPanel::showRecord() {
    int brow = m_blockCombo->currentIndex();
    if (brow < 0 || brow >= (int)m_blocks.size()) return;
    const auto& b = m_blocks[brow];
    int part = m_partCombo->currentIndex();
    const sb::QmdBlock* show = &b;
    if (part > 0 && part <= (int)b.parts.size()) show = &b.parts[part - 1];

    int idx = m_recSpin->value();
    if (idx < 0 || idx >= (int)show->faces.size()) return;
    const auto& face = show->faces[idx];
    m_model->setSelectedFace(idx);
    m_uv->setSelected(idx);

    // decode with the active spec for the record's type
    std::string text;
    auto it = sb::g_parseOpts.recSpecs.find(face.recType);
    if (it != sb::g_parseOpts.recSpecs.end()) text = it->second;
    else {
        auto dit = sb::defaultRecSpecs().find(face.recType);
        if (dit == sb::defaultRecSpecs().end()) return;
        text = dit->second;
    }
    auto spec = sb::parseRecSpec(text);
    if (!spec.valid || face.recOff + spec.size > m_bytes.size()) return;
    auto rv = sb::decodeRecord(spec, m_bytes.data() + face.recOff);
    m_hex->setSelection(face.recOff, spec.size);

    m_recTable->setRowCount((int)spec.fields.size());
    int row = 0;
    for (const auto& f : spec.fields) {
        uint32_t v = rv.v.at(f.role);
        m_recTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(f.role)));
        m_recTable->setItem(row, 1, new QTableWidgetItem(QString("%1 (0x%2)").arg(v).arg(v, 0, 16)));
        QString raw;
        for (int k = 0; k < f.size; ++k)
            raw += QString("%1 ").arg(m_bytes[face.recOff + f.offset + k], 2, 16, QLatin1Char('0'));
        m_recTable->setItem(row, 2, new QTableWidgetItem(raw.trimmed()));
        ++row;
    }
    m_recTable->resizeColumnsToContents();
}
