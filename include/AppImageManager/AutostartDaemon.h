#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QHash>
#include <QByteArray>
#include <QSet>
#include <QString>
#include <QTimer>

namespace appimagemanager {

class AppImageManager;

class AutostartDaemon : public QObject {
    Q_OBJECT
public:
    explicit AutostartDaemon(AppImageManager &manager, QObject *parent = nullptr);

public slots:
    void start();

private slots:
    void onManifestChanged(const QString &path);
    void onManifestDirectoryChanged(const QString &path);
    void onSyncTimeout();
    void onDisplayTimerTimeout();

private:
    void scheduleSync();
    void performSync();
    void updateManifestWatch();
    bool ensureDisplayReady();
    bool refreshDisplayEnvironment();
    void applyEnvironmentVariable(const QString &key, const QString &value);

private:
    AppImageManager &m_manager;
    QFileSystemWatcher m_watcher;
    QTimer m_syncTimer;
    QTimer m_displayTimer;
    QSet<QString> m_startedIds;
    QString m_manifestPath;
    QString m_manifestDirectory;
    bool m_displayReady;
    bool m_pendingSync;
    QHash<QString, QByteArray> m_environmentCache;
};

} // namespace appimagemanager
