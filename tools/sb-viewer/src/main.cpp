#include <QApplication>
#include <QFile>
#include "ui/MainWindow.hpp"
#include "ui/ModelView.hpp"
#include "ui/LabPanel.hpp"
#include <QVBoxLayout>
#include <QTimer>
#include "formats/QmdModel.hpp"

// Headless verification: sb-viewer --render <part3 file> <block name> <out.png>
static int renderCli(const QString& path, const QString& blockName, const QString& out) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return 1;
    QByteArray raw = f.readAll();
    auto blocks = sb::parseQmdContainer((const uint8_t*)raw.constData(), raw.size());
    sb::TextureBank tex;
    QString p1 = path;
    p1.replace(".part3_raw.bin", ".part1_rle.bin");
    QFile f1(p1);
    if (f1.open(QIODevice::ReadOnly)) {
        QByteArray raw1 = f1.readAll();
        tex.load((const uint8_t*)raw1.constData(), raw1.size());
    }
    for (const auto& b : blocks) {
        if (b.verts.empty()) continue;
        if (QString::fromStdString(b.name) != blockName) continue;
        ModelView view;
        view.resize(640, 640);
        view.setModel(b.verts, b.faces, &tex);
        QImage img = view.grab().toImage();
        if (!img.save(out)) return 1;
        printf("rendered %s: %zu verts, %zu tris -> %s\n",
               b.name.c_str(), b.verts.size(), b.faces.size(), qPrintable(out));
        return 0;
    }
    fprintf(stderr, "block %s not found or empty\n", qPrintable(blockName));
    return 1;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    if (argc == 5 && QString(argv[1]) == "--render")
        return renderCli(QString::fromUtf8(argv[2]), QString::fromUtf8(argv[3]),
                         QString::fromUtf8(argv[4]));
    QStringList argvL; for (int i = 0; i < argc; ++i) argvL << argv[i];
    if (argvL.contains("--lab")) {
        // model lab: --lab [part3 file] [block name]
        const QStringList& args = argvL;
        int li = args.indexOf("--lab");
        QString path = li + 1 < args.size() && !args[li + 1].startsWith("--")
                           ? args[li + 1]
                           : "/Users/alexeykrasnopolsky/Desktop/silent_bomber/experiments/out/A00.part3_raw.bin";
        QString block = li + 2 < args.size() && !args[li + 2].startsWith("--")
                            ? args[li + 2]
                            : "CMFANR00";
        QWidget w;
        w.setWindowTitle("SB model lab — " + path);
        auto* l = new QVBoxLayout(&w);
        l->setContentsMargins(0, 0, 0, 0);
        auto* lab = new LabPanel(&w);
        l->addWidget(lab);
        w.resize(1400, 900);
        w.show();
        lab->loadFile(path, block);
        if (const char* shot = getenv("SB_SHOT")) {  // test hook: grab & quit
            QTimer::singleShot(600, &w, [shot, &w]() {
                w.grab().save(QString::fromUtf8(shot));
                QApplication::quit();
            });
        }
        return app.exec();
    }
    MainWindow w;
    w.show();
    return app.exec();
}
