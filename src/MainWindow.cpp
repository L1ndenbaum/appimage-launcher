#include "AppImageManager/MainWindow.h"

#include <QAction>
#include <QAbstractItemView>
#include <QDesktopServices>
#include <QFileDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QProcess>
#include <QToolBar>
#include <QUrl>

#include <filesystem>
#include <stdexcept>

namespace appimagelauncher {

MainWindow::MainWindow(AppImageManager &manager, QWidget *parent)
    : QMainWindow(parent)
    , m_manager(manager)
    , m_listWidget(nullptr)
    , m_addAction(nullptr)
    , m_removeAction(nullptr)
    , m_openAction(nullptr)
    , m_openStorageAction(nullptr)
    , m_autostartAction(nullptr)
{
    createUi();
    refreshEntries();
    setWindowTitle(tr("AppImage Manager"));
}

void MainWindow::createUi()
{
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    setCentralWidget(m_listWidget);

    auto *toolBar = addToolBar(tr("Actions"));

    m_addAction = toolBar->addAction(tr("Add"));
    connect(m_addAction, &QAction::triggered, this, &MainWindow::onAddAppImage);

    m_openAction = toolBar->addAction(tr("Open"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenSelected);

    m_removeAction = toolBar->addAction(tr("Remove"));
    connect(m_removeAction, &QAction::triggered, this, &MainWindow::onRemoveSelected);

    m_autostartAction = toolBar->addAction(tr("Enable Autostart"));
    connect(m_autostartAction, &QAction::triggered, this, &MainWindow::onToggleAutostart);

    m_openStorageAction = toolBar->addAction(tr("Open Storage"));
    connect(m_openStorageAction, &QAction::triggered, this, &MainWindow::onOpenStorageDirectory);

    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) { onOpenSelected(); });
    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, [this]() { updateActionsForSelection(); });

    updateActionsForSelection();
}

void MainWindow::refreshEntries()
{
    const auto currentSelection = m_listWidget->currentItem() ? m_listWidget->currentItem()->data(Qt::UserRole).toString() : QString();

    m_listWidget->clear();
    const auto entries = m_manager.entries();
    for (const auto &entry : entries) {
        QString displayName = QString::fromStdString(entry.name);
        if (entry.autostart) {
            displayName += tr(" (Autostart)");
        }
        auto *item = new QListWidgetItem(displayName);
        item->setData(Qt::UserRole, QString::fromStdString(entry.id));
        item->setToolTip(QString::fromStdString(entry.storedPath.string()));
        m_listWidget->addItem(item);

        if (!currentSelection.isEmpty() && currentSelection == item->data(Qt::UserRole).toString()) {
            m_listWidget->setCurrentItem(item);
        }
    }

    updateActionsForSelection();
}

void MainWindow::onAddAppImage()
{
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Select AppImage"), QString(), tr("AppImage Files (*.AppImage);;All Files (*)"));
    if (filePath.isEmpty()) {
        return;
    }

    try {
        m_manager.addAppImage(std::filesystem::u8path(filePath.toUtf8().constData()));
        refreshEntries();
    } catch (const std::exception &error) {
        QMessageBox::critical(this, tr("Unable to add AppImage"), QString::fromUtf8(error.what()));
    }
}

void MainWindow::onRemoveSelected()
{
    const auto selectedItems = m_listWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    const auto *item = selectedItems.front();
    const QString id = item->data(Qt::UserRole).toString();

    if (QMessageBox::question(this, tr("Remove AppImage"), tr("Do you really want to remove the selected AppImage?")) != QMessageBox::Yes) {
        return;
    }

    try {
        m_manager.removeAppImage(id.toStdString());
        refreshEntries();
    } catch (const std::exception &error) {
        QMessageBox::critical(this, tr("Unable to remove"), QString::fromUtf8(error.what()));
    }
}

void MainWindow::onOpenSelected()
{
    const auto selectedItems = m_listWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    const auto *item = selectedItems.front();
    const QString id = item->data(Qt::UserRole).toString();
    const auto entry = m_manager.entryById(id.toStdString());
    if (!entry.has_value()) {
        QMessageBox::warning(this, tr("Launch failed"), tr("Unable to locate the stored AppImage."));
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
    const auto selectedItems = m_listWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    const auto *item = selectedItems.front();
    const QString id = item->data(Qt::UserRole).toString();
    const auto entry = m_manager.entryById(id.toStdString());
    if (!entry.has_value()) {
        QMessageBox::warning(this, tr("Missing AppImage"), tr("Unable to locate the selected AppImage."));
        return;
    }

    const bool enable = !entry->autostart;
    try {
        m_manager.setAutostart(id.toStdString(), enable);
        refreshEntries();
    } catch (const std::exception &error) {
        QMessageBox::critical(this, tr("Unable to update autostart"), QString::fromUtf8(error.what()));
    }
}

void MainWindow::updateActionsForSelection()
{
    if (!m_autostartAction) {
        return;
    }

    const bool hasSelection = !m_listWidget->selectedItems().isEmpty();
    m_removeAction->setEnabled(hasSelection);
    m_openAction->setEnabled(hasSelection);

    if (!hasSelection) {
        m_autostartAction->setEnabled(false);
        m_autostartAction->setText(tr("Enable Autostart"));
        return;
    }

    const auto *item = m_listWidget->selectedItems().front();
    const QString id = item->data(Qt::UserRole).toString();
    const auto entry = m_manager.entryById(id.toStdString());
    if (!entry.has_value()) {
        m_autostartAction->setEnabled(false);
        m_autostartAction->setText(tr("Enable Autostart"));
        return;
    }

    m_autostartAction->setEnabled(true);
    m_autostartAction->setText(entry->autostart ? tr("Disable Autostart") : tr("Enable Autostart"));
}

} // namespace appimagelauncher
