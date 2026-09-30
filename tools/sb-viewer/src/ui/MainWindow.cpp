#include "MainWindow.hpp"
#include "ViewportPanel.hpp"

#include <QFileDialog>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>

MainWindow::MainWindow() {
    setWindowTitle("Silent Bomber Viewer");
    resize(1200, 800);

    auto* openAct = new QAction("&Open package dir…", this);
    connect(openAct, &QAction::triggered, this, &MainWindow::openDirectory);
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(openAct);

    auto* splitter = new QSplitter(this);
    m_tree = new QTreeWidget(splitter);
    m_tree->setHeaderLabels({"Name", "Info"});
    m_viewport = new ViewportPanel(splitter);
    splitter->addWidget(m_tree);
    splitter->addWidget(m_viewport);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);

    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        auto items = m_tree->selectedItems();
        if (items.isEmpty()) return;
        QString path = items.first()->data(0, Qt::UserRole).toString();
        if (!path.isEmpty()) m_viewport->showFile(path);
    });

    statusBar()->showMessage("Open a directory with extracted packages (experiments/out)");
}

void MainWindow::openDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Open package directory", m_rootDir);
    if (dir.isEmpty()) return;
    m_rootDir = dir;
    populateTree(dir);
    statusBar()->showMessage(dir);
}

void MainWindow::populateTree(const QString& root) {
    m_tree->clear();
    QDir dir(root);
    // group by package name (strip .partN_xxx.bin suffix)
    QMap<QString, QStringList> packages;
    for (const auto& fi : dir.entryInfoList(QDir::Files, QDir::Name)) {
        QString name = fi.fileName();
        int dot = name.indexOf(".part");
        QString pkg = dot > 0 ? name.left(dot) : name;
        packages[pkg].append(fi.absoluteFilePath());
    }
    for (auto it = packages.begin(); it != packages.end(); ++it) {
        auto* pkgItem = new QTreeWidgetItem(m_tree, {it.key()});
        for (const QString& p : it.value()) {
            QFileInfo fi(p);
            auto* child = new QTreeWidgetItem(pkgItem, {fi.fileName(), QString::number(fi.size())});
            child->setData(0, Qt::UserRole, p);
        }
        pkgItem->setExpanded(false);
    }
}
