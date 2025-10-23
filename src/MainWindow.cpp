#include "AppImageManager/MainWindow.h"

#include "AppImageManager/SettingsDialog.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QInputDialog>
#include <QListView>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLineEdit>
#include <QColor>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace appimagelauncher {

namespace {
constexpr int kIdRole = Qt::UserRole;
constexpr int kAutostartRole = Qt::UserRole + 1;

QString initialsForName(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        return QStringLiteral("A");
    }

    QString initials;
    bool takeNext = true;
    for (const QChar &ch : trimmed) {
        if (ch.isSpace()) {
            takeNext = true;
            continue;
        }
        if (takeNext) {
            initials.append(ch);
            takeNext = false;
            if (initials.size() >= 2) {
                break;
            }
        }
    }

    if (initials.isEmpty()) {
        initials = trimmed.left(2);
    }

    return initials.toUpper();
}

QColor accentColorForId(const std::string &id)
{
    const std::size_t hash = std::hash<std::string> {}(id);
    const int hue = static_cast<int>(hash % 360);
    QColor color;
    color.setHsl(hue, 150, 140);
    return color;
}

QIcon themedIcon(const QString &name, QStyle::StandardPixmap fallback)
{
    QIcon icon = QIcon::fromTheme(name);
    if (icon.isNull()) {
        icon = qApp->style()->standardIcon(fallback);
    }
    return icon;
}

} // namespace

MainWindow::MainWindow(AppImageManager &manager, TranslationManager &translator, Preferences preferences, QWidget *parent)
    : QMainWindow(parent)
    , m_manager(manager)
    , m_translationManager(translator)
    , m_preferences(std::move(preferences))
    , m_listWidget(nullptr)
    , m_addAction(nullptr)
    , m_removeAction(nullptr)
    , m_openAction(nullptr)
    , m_openStorageAction(nullptr)
    , m_autostartAction(nullptr)
    , m_renameAction(nullptr)
    , m_settingsAction(nullptr)
    , m_quitAction(nullptr)
    , m_viewListAction(nullptr)
    , m_viewGridAction(nullptr)
    , m_actionToolBar(nullptr)
    , m_fileMenu(nullptr)
    , m_viewMenu(nullptr)
    , m_preferencesMenu(nullptr)
    , m_viewActions(nullptr)
{
    createUi();
    retranslateUi();
    applyViewMode();

    // Apply language preference before populating content.
    if (m_translationManager.applyLanguage(m_preferences.language)) {
        retranslateUi();
    }

    refreshEntries();
    updateActionsForSelection();

    setMinimumSize(720, 460);
    setUnifiedTitleAndToolBarOnMac(true);
}

void MainWindow::createUi()
{
    createToolBar();
    createMenus();

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    m_listWidget = new QListWidget(central);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setAlternatingRowColors(true);
    m_listWidget->setUniformItemSizes(false);
    m_listWidget->setSpacing(6);
    m_listWidget->setIconSize(QSize(72, 72));
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_listWidget);

    setCentralWidget(central);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) { onOpenSelected(); });
    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, [this]() { updateActionsForSelection(); });
    connect(m_listWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::onContextMenuRequested);
}

void MainWindow::createToolBar()
{
    m_actionToolBar = addToolBar(QString());
    m_actionToolBar->setMovable(false);
    m_actionToolBar->setIconSize(QSize(22, 22));

    m_addAction = m_actionToolBar->addAction(themedIcon(QStringLiteral("list-add"), QStyle::SP_DialogOpenButton), QString());
    connect(m_addAction, &QAction::triggered, this, &MainWindow::onAddAppImage);

    m_openAction = m_actionToolBar->addAction(themedIcon(QStringLiteral("system-run"), QStyle::SP_MediaPlay), QString());
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenSelected);

    m_renameAction = m_actionToolBar->addAction(themedIcon(QStringLiteral("edit-rename"), QStyle::SP_FileDialogDetailedView), QString());
    connect(m_renameAction, &QAction::triggered, this, &MainWindow::onRenameSelected);

    m_removeAction = m_actionToolBar->addAction(themedIcon(QStringLiteral("edit-delete"), QStyle::SP_TrashIcon), QString());
    connect(m_removeAction, &QAction::triggered, this, &MainWindow::onRemoveSelected);

    m_actionToolBar->addSeparator();

    m_autostartAction = m_actionToolBar->addAction(themedIcon(QStringLiteral("media-playback-start"), QStyle::SP_BrowserReload), QString());
    connect(m_autostartAction, &QAction::triggered, this, &MainWindow::onToggleAutostart);

    m_openStorageAction = m_actionToolBar->addAction(themedIcon(QStringLiteral("folder-open"), QStyle::SP_DirOpenIcon), QString());
    connect(m_openStorageAction, &QAction::triggered, this, &MainWindow::onOpenStorageDirectory);

    m_actionToolBar->addSeparator();

    m_settingsAction = m_actionToolBar->addAction(themedIcon(QStringLiteral("preferences-system"), QStyle::SP_FileDialogListView), QString());
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onOpenPreferences);
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(QString());
    m_fileMenu->addAction(m_addAction);
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_renameAction);
    m_fileMenu->addAction(m_autostartAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_removeAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_openStorageAction);
    m_fileMenu->addSeparator();

    m_quitAction = new QAction(this);
    connect(m_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    m_fileMenu->addAction(m_quitAction);

    m_viewMenu = menuBar()->addMenu(QString());
    m_viewActions = new QActionGroup(this);
    m_viewActions->setExclusive(true);

    m_viewListAction = new QAction(this);
    m_viewListAction->setCheckable(true);
    m_viewListAction->setData(static_cast<int>(ViewMode::List));
    connect(m_viewListAction, &QAction::triggered, this, [this]() {
        if (m_preferences.viewMode != ViewMode::List) {
            m_preferences.viewMode = ViewMode::List;
            applyViewMode();
            refreshEntries();
            m_preferences.save();
        }
    });

    m_viewGridAction = new QAction(this);
    m_viewGridAction->setCheckable(true);
    m_viewGridAction->setData(static_cast<int>(ViewMode::Grid));
    connect(m_viewGridAction, &QAction::triggered, this, [this]() {
        if (m_preferences.viewMode != ViewMode::Grid) {
            m_preferences.viewMode = ViewMode::Grid;
            applyViewMode();
            refreshEntries();
            m_preferences.save();
        }
    });

    m_viewActions->addAction(m_viewListAction);
    m_viewActions->addAction(m_viewGridAction);

    m_viewMenu->addAction(m_viewListAction);
    m_viewMenu->addAction(m_viewGridAction);

    m_preferencesMenu = menuBar()->addMenu(QString());
    m_preferencesMenu->addAction(m_settingsAction);
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("AppImage Manager"));

    if (m_actionToolBar) {
        m_actionToolBar->setWindowTitle(tr("Actions"));
    }

    if (m_fileMenu) {
        m_fileMenu->setTitle(tr("File"));
    }
    if (m_viewMenu) {
        m_viewMenu->setTitle(tr("View"));
    }
    if (m_preferencesMenu) {
        m_preferencesMenu->setTitle(tr("Settings"));
    }

    if (m_addAction) {
        m_addAction->setText(tr("Add"));
        m_addAction->setToolTip(tr("Add a new AppImage"));
    }
    if (m_openAction) {
        m_openAction->setText(tr("Open"));
        m_openAction->setToolTip(tr("Launch the selected AppImage"));
    }
    if (m_renameAction) {
        m_renameAction->setText(tr("Rename"));
        m_renameAction->setToolTip(tr("Rename the selected AppImage"));
    }
    if (m_removeAction) {
        m_removeAction->setText(tr("Remove"));
        m_removeAction->setToolTip(tr("Remove the selected AppImage"));
    }
    if (m_autostartAction) {
        m_autostartAction->setToolTip(tr("Toggle autostart for the selected AppImage"));
    }
    if (m_openStorageAction) {
        m_openStorageAction->setText(tr("Open Storage"));
        m_openStorageAction->setToolTip(tr("Show the managed storage directory"));
    }
    if (m_settingsAction) {
        m_settingsAction->setText(tr("Preferences"));
        m_settingsAction->setToolTip(tr("Open application settings"));
    }
    if (m_quitAction) {
        m_quitAction->setText(tr("Quit"));
        m_quitAction->setToolTip(tr("Quit AppImage Manager"));
    }
    if (m_viewListAction) {
        m_viewListAction->setText(tr("List view"));
    }
    if (m_viewGridAction) {
        m_viewGridAction->setText(tr("Grid view"));
    }

    updateActionsForSelection();
}

void MainWindow::applyViewMode()
{
    if (!m_listWidget) {
        return;
    }

    const bool grid = m_preferences.viewMode == ViewMode::Grid;
    m_listWidget->setViewMode(grid ? QListView::IconMode : QListView::ListMode);
    m_listWidget->setResizeMode(grid ? QListView::Adjust : QListView::Fixed);
    m_listWidget->setWrapping(grid);
    m_listWidget->setWordWrap(grid);
    m_listWidget->setSpacing(grid ? 16 : 6);
    m_listWidget->setIconSize(grid ? QSize(96, 96) : QSize(48, 48));
    m_listWidget->setGridSize(grid ? QSize(200, 160) : QSize());

    if (m_viewListAction && m_viewGridAction) {
        m_viewListAction->setChecked(!grid);
        m_viewGridAction->setChecked(grid);
    }
}

void MainWindow::refreshEntries()
{
    if (!m_listWidget) {
        return;
    }

    const QString currentId = [&]() -> QString {
        const auto items = m_listWidget->selectedItems();
        if (!items.isEmpty()) {
            return items.front()->data(kIdRole).toString();
        }
        return QString();
    }();

    m_listWidget->clear();

    auto entries = m_manager.entries();
    std::sort(entries.begin(), entries.end(), [](const AppImageEntry &lhs, const AppImageEntry &rhs) {
        return QString::fromStdString(lhs.name).localeAwareCompare(QString::fromStdString(rhs.name)) < 0;
    });

    for (const auto &entry : entries) {
        auto *item = new QListWidgetItem();
        item->setData(kIdRole, QString::fromStdString(entry.id));
        item->setData(kAutostartRole, entry.autostart);
        rebuildItem(item, entry);
        m_listWidget->addItem(item);

        if (!currentId.isEmpty() && currentId == QString::fromStdString(entry.id)) {
            m_listWidget->setCurrentItem(item);
        }
    }

    updateActionsForSelection();

    const int count = static_cast<int>(entries.size());
    statusBar()->showMessage(tr("%n AppImage(s) managed", "", count));
}

void MainWindow::rebuildItem(QListWidgetItem *item, const AppImageEntry &entry)
{
    item->setText(decoratedName(entry));
    item->setIcon(avatarForEntry(entry));
    item->setToolTip(QString::fromStdString(entry.storedPath.string()));

    if (m_preferences.viewMode == ViewMode::Grid) {
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    } else {
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    }
}

QIcon MainWindow::avatarForEntry(const AppImageEntry &entry) const
{
    QFileInfo fileInfo(QString::fromStdString(entry.storedPath.string()));
    QFileIconProvider provider;
    QIcon icon = provider.icon(fileInfo);
    if (!icon.isNull()) {
        return icon;
    }

    const QString initials = initialsForName(QString::fromStdString(entry.name));
    const QColor background = accentColorForId(entry.id);
    const int size = m_preferences.viewMode == ViewMode::Grid ? 128 : 64;

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setBrush(background);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(pixmap.rect().adjusted(2, 2, -2, -2));

    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(size / 2);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, initials);

    return QIcon(pixmap);
}

QString MainWindow::decoratedName(const AppImageEntry &entry) const
{
    QString text = QString::fromStdString(entry.name);
    if (entry.autostart) {
        text += tr(" (Autostart)");
    }
    return text;
}

void MainWindow::updateActionsForSelection()
{
    const auto entry = selectedEntry();
    const bool hasSelection = entry.has_value();

    if (m_openAction) {
        m_openAction->setEnabled(hasSelection);
    }
    if (m_removeAction) {
        m_removeAction->setEnabled(hasSelection);
    }
    if (m_autostartAction) {
        if (hasSelection) {
            m_autostartAction->setEnabled(true);
            m_autostartAction->setText(entry->autostart ? tr("Disable Autostart") : tr("Enable Autostart"));
        } else {
            m_autostartAction->setEnabled(false);
            m_autostartAction->setText(tr("Enable Autostart"));
        }
    }
    if (m_renameAction) {
        m_renameAction->setEnabled(hasSelection);
    }
    if (m_settingsAction) {
        m_settingsAction->setEnabled(true);
    }

    if (m_viewListAction && m_viewGridAction) {
        m_viewListAction->setChecked(m_preferences.viewMode == ViewMode::List);
        m_viewGridAction->setChecked(m_preferences.viewMode == ViewMode::Grid);
    }
}

std::optional<AppImageEntry> MainWindow::selectedEntry() const
{
    if (!m_listWidget) {
        return std::nullopt;
    }

    const auto items = m_listWidget->selectedItems();
    if (items.isEmpty()) {
        return std::nullopt;
    }

    const QString id = items.front()->data(kIdRole).toString();
    if (id.isEmpty()) {
        return std::nullopt;
    }

    return m_manager.entryById(id.toStdString());
}

void MainWindow::promptAutostartFailure(const std::exception &error)
{
    QMessageBox::critical(this, tr("Unable to update autostart"), QString::fromUtf8(error.what()));
}

void MainWindow::applyPreferences(const Preferences &preferences)
{
    m_preferences = preferences;
    const bool languageChanged = m_translationManager.applyLanguage(m_preferences.language);

    applyViewMode();
    refreshEntries();
    if (languageChanged) {
        retranslateUi();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
        refreshEntries();
    }
}

void MainWindow::onAddAppImage()
{
    const QString filePath = QFileDialog::getOpenFileName(this,
        tr("Select AppImage"),
        QString(),
        tr("AppImage Files (*.AppImage);;All Files (*)"));
    if (filePath.isEmpty()) {
        return;
    }

    try {
        m_manager.addAppImage(std::filesystem::u8path(filePath.toUtf8().constData()), m_preferences.moveToStorageOnAdd);
        refreshEntries();
    } catch (const std::exception &error) {
        QMessageBox::critical(this, tr("Unable to add AppImage"), QString::fromUtf8(error.what()));
    }
}

void MainWindow::onRemoveSelected()
{
    const auto entry = selectedEntry();
    if (!entry.has_value()) {
        return;
    }

    if (m_preferences.confirmRemoval) {
        if (QMessageBox::question(this, tr("Remove AppImage"), tr("Do you really want to remove the selected AppImage?")) != QMessageBox::Yes) {
            return;
        }
    }

    try {
        m_manager.removeAppImage(entry->id);
        refreshEntries();
    } catch (const std::exception &error) {
        QMessageBox::critical(this, tr("Unable to remove"), QString::fromUtf8(error.what()));
    }
}

void MainWindow::onOpenSelected()
{
    const auto entry = selectedEntry();
    if (!entry.has_value()) {
        return;
    }

    const QString executable = QString::fromStdString(entry->storedPath.string());
    if (!QProcess::startDetached(executable, {})) {
        QMessageBox::warning(this, tr("Launch failed"), tr("Unable to start the AppImage."));
    }
}

void MainWindow::onOpenStorageDirectory()
{
    const auto url = QUrl::fromLocalFile(QString::fromStdString(m_manager.storageDirectory().string()));
    QDesktopServices::openUrl(url);
}

void MainWindow::onToggleAutostart()
{
    const auto entry = selectedEntry();
    if (!entry.has_value()) {
        return;
    }

    try {
        m_manager.setAutostart(entry->id, !entry->autostart);
        refreshEntries();
    } catch (const std::exception &error) {
        promptAutostartFailure(error);
    }
}

void MainWindow::onRenameSelected()
{
    const auto entry = selectedEntry();
    if (!entry.has_value()) {
        return;
    }

    bool accepted = false;
    const QString currentName = QString::fromStdString(entry->name);
    const QString newName = QInputDialog::getText(this,
        tr("Rename AppImage"),
        tr("New name"),
        QLineEdit::Normal,
        currentName,
        &accepted);
    if (!accepted) {
        return;
    }

    const QString trimmed = newName.trimmed();
    if (trimmed.isEmpty()) {
        QMessageBox::warning(this, tr("Rename AppImage"), tr("The name must not be empty."));
        return;
    }

    if (trimmed == currentName) {
        return;
    }

    try {
        m_manager.renameAppImage(entry->id, trimmed.toStdString());
        refreshEntries();
    } catch (const std::exception &error) {
        QMessageBox::critical(this, tr("Unable to rename AppImage"), QString::fromUtf8(error.what()));
    }
}

void MainWindow::onOpenPreferences()
{
    SettingsDialog dialog(m_preferences, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    Preferences updated = dialog.preferences();
    if (updated.moveToStorageOnAdd == m_preferences.moveToStorageOnAdd
        && updated.confirmRemoval == m_preferences.confirmRemoval
        && updated.viewMode == m_preferences.viewMode
        && updated.language == m_preferences.language) {
        return;
    }

    applyPreferences(updated);
    m_preferences.save();
}

void MainWindow::onContextMenuRequested(const QPoint &position)
{
    const auto entry = selectedEntry();
    if (!entry.has_value()) {
        return;
    }

    QMenu menu(this);
    menu.addAction(m_openAction);
    menu.addAction(m_renameAction);
    menu.addAction(m_autostartAction);
    menu.addSeparator();
    menu.addAction(m_removeAction);
    menu.exec(m_listWidget->viewport()->mapToGlobal(position));
}

} // namespace appimagelauncher
