#pragma once

#include <QMainWindow>
#include <QTreeWidget>
#include <QSplitter>

class ViewportPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

private:
    void openDirectory();
    void populateTree(const QString& root);

    QTreeWidget* m_tree = nullptr;
    ViewportPanel* m_viewport = nullptr;
    QString m_rootDir;
};
