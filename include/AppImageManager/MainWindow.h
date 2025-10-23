#pragma once

#include <QMainWindow>

#include "AppImageManager/AppImageManager.h"

QT_BEGIN_NAMESPACE
class QListWidget;
class QAction;
QT_END_NAMESPACE

namespace appimagelauncher {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppImageManager &manager, QWidget *parent = nullptr);

private slots:
    void onAddAppImage();
    void onRemoveSelected();
    void onOpenSelected();
    void onOpenStorageDirectory();
    void onToggleAutostart();

private:
    void createUi();
    void refreshEntries();
    void updateActionsForSelection();

private:
    AppImageManager &m_manager;
    QListWidget *m_listWidget;
    QAction *m_addAction;
    QAction *m_removeAction;
    QAction *m_openAction;
    QAction *m_openStorageAction;
    QAction *m_autostartAction;
};

} // namespace appimagelauncher

