#include <QApplication>
#include <QFile>
#include "ui/MainWindow.hpp"
#include "ui/ModelView.hpp"
#include "formats/QmdModel.hpp"

// Headless verification: sb-viewer --render <part3 file> <block name> <out.png>
static int renderCli(const QString& path, const QString& blockName, const QString& out) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return 1;
    QByteArray raw = f.readAll();
    auto blocks = sb::parseQmdContainer((const uint8_t*)raw.constData(), raw.size());
    for (const auto& b : blocks) {
        if (b.verts.empty()) continue;
        if (QString::fromStdString(b.name) != blockName) continue;
        ModelView view;
        view.resize(640, 640);
        view.setModel(b.verts, b.faces);
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
    MainWindow w;
    w.show();
    return app.exec();
}
