#pragma once

#include <QMainWindow>

#include "AppImageManager/AppImageManager.h"
#include "AppImageManager/Preferences.h"
#include "AppImageManager/TranslationManager.h"

QT_BEGIN_NAMESPACE
class QListWidget;
class QAction;
class QListWidgetItem;
class QMenu;
class QToolBar;
class QActionGroup;
QT_END_NAMESPACE

namespace appimagemanager {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(AppImageManager &manager, TranslationManager &translator, Preferences preferences, QWidget *parent = nullptr);

    Preferences preferences() const { return m_preferences; }
    void applyPreferences(const Preferences &preferences);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onAddAppImage();
    void onRemoveSelected();
    void onOpenSelected();
    void onOpenStorageDirectory();
    void onToggleAutostart();
    void onRenameSelected();
    void onOpenPreferences();
    void onContextMenuRequested(const QPoint &position);

private:
    void createUi();
    void createMenus();
    void createToolBar();
    void retranslateUi();
    void applyViewMode();
    void refreshEntries();
    void rebuildItem(QListWidgetItem *item, const AppImageEntry &entry);
    QIcon avatarForEntry(const AppImageEntry &entry) const;
    QString decoratedName(const AppImageEntry &entry) const;
    void updateActionsForSelection();
    std::optional<AppImageEntry> selectedEntry() const;
    void promptAutostartFailure(const std::exception &error);

private:
    AppImageManager &m_manager;
    TranslationManager &m_translationManager;
    Preferences m_preferences;
    QListWidget *m_listWidget;
    QAction *m_addAction;
    QAction *m_removeAction;
    QAction *m_openAction;
    QAction *m_openStorageAction;
    QAction *m_autostartAction;
    QAction *m_renameAction;
    QAction *m_settingsAction;
    QAction *m_quitAction;
    QAction *m_viewListAction;
    QAction *m_viewGridAction;
    QToolBar *m_actionToolBar;
    QMenu *m_fileMenu;
    QMenu *m_viewMenu;
    QMenu *m_preferencesMenu;
    QActionGroup *m_viewActions;
};

} // namespace appimagemanager
