#include "AppImageManager/AutostartDaemon.h"

#include "AppImageManager/AppImageManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <QThread>
#include <exception>

namespace appimagemanager {

namespace {
QString toQString(const std::filesystem::path &path)
{
    return QString::fromStdString(path.string());
}

using EnvironmentMap = QHash<QString, QString>;

EnvironmentMap readSystemdUserEnvironment()
{
    EnvironmentMap env;

    QProcess process;
    process.start(QStringLiteral("systemctl"), {QStringLiteral("--user"), QStringLiteral("show-environment")});
    if (!process.waitForStarted(1000)) {
        return env;
    }
    if (!process.waitForFinished(2000)) {
        process.kill();
        process.waitForFinished();
        return env;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return env;
    }
    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    const auto lines = output.split(QChar('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const int equalsIndex = line.indexOf(QLatin1Char('='));
        if (equalsIndex <= 0) {
            continue;
        }

        const QString key = line.left(equalsIndex).trimmed();
        const QString value = line.mid(equalsIndex + 1);
        if (key.isEmpty()) {
            continue;
        }

        env.insert(key, value);
    }

    return env;
}

QString probeX11Display()
{
    QDir dir(QStringLiteral("/tmp/.X11-unix"));
    if (!dir.exists()) {
        return {};
    }

    const QStringList entries = dir.entryList(QStringList() << QStringLiteral("X*"), QDir::System | QDir::Files | QDir::NoDotAndDotDot);
    if (entries.isEmpty()) {
        return {};
    }

    const QString candidate = entries.first();
    const QString number = candidate.mid(1);
    if (number.isEmpty()) {
        return {};
    }
    return number.startsWith(QLatin1Char(':')) ? number : QStringLiteral(":%1").arg(number);
}

QString probeWaylandDisplay()
{
    const QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (runtimeDir.isEmpty()) {
        return {};
    }

    QDir dir(runtimeDir);
    if (!dir.exists()) {
        return {};
    }

    const QStringList entries = dir.entryList(QStringList() << QStringLiteral("wayland-*"), QDir::System | QDir::Files | QDir::NoDotAndDotDot);
    if (entries.isEmpty()) {
        return {};
    }

    return entries.first();
}
} // namespace

AutostartDaemon::AutostartDaemon(AppImageManager &manager, QObject *parent)
    : QObject(parent)
    , m_manager(manager)
    , m_displayReady(false)
    , m_pendingSync(false)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &AutostartDaemon::onManifestChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &AutostartDaemon::onManifestDirectoryChanged);

    m_syncTimer.setSingleShot(true);
    m_syncTimer.setInterval(250);
    connect(&m_syncTimer, &QTimer::timeout, this, &AutostartDaemon::onSyncTimeout);

    m_displayTimer.setParent(this);
    m_displayTimer.setInterval(1500);
    connect(&m_displayTimer, &QTimer::timeout, this, &AutostartDaemon::onDisplayTimerTimeout);
}

void AutostartDaemon::start()
{
    qInfo() << "[autostart-daemon] initializing...";
    ensureDisplayReady();

    m_manifestPath = toQString(m_manager.manifestPath());
    if (!m_manifestPath.isEmpty()) {
        QFileInfo manifestInfo(m_manifestPath);
        m_manifestDirectory = manifestInfo.absolutePath();
        if (!m_manifestDirectory.isEmpty()) {
            if (!m_watcher.directories().contains(m_manifestDirectory)) {
                m_watcher.addPath(m_manifestDirectory);
            }
        }
        updateManifestWatch();
    }

    scheduleSync();
}

void AutostartDaemon::onManifestChanged(const QString &path)
{
    Q_UNUSED(path);
    updateManifestWatch();
    scheduleSync();
}

void AutostartDaemon::onManifestDirectoryChanged(const QString &path)
{
    Q_UNUSED(path);
    updateManifestWatch();
    scheduleSync();
}

void AutostartDaemon::onSyncTimeout()
{
    performSync();
}

void AutostartDaemon::scheduleSync()
{
    if (!ensureDisplayReady()) {
        m_pendingSync = true;
        return;
    }

    if (!m_syncTimer.isActive()) {
        m_syncTimer.start();
    }
}

void AutostartDaemon::performSync()
{
    if (!ensureDisplayReady()) {
        m_pendingSync = true;
        return;
    }

    QSet<QString> desiredIds;
    try {
        m_manager.load();
        const auto entries = m_manager.entries();
        for (const auto &entry : entries) {
            if (!entry.autostart) {
                continue;
            }

            const QString id = QString::fromStdString(entry.id);
            desiredIds.insert(id);
            if (m_startedIds.contains(id)) {
                continue;
            }

            const QString executable = toQString(entry.storedPath);
            if (executable.isEmpty()) {
                continue;
            }

            if (!QFileInfo::exists(executable)) {
                qWarning() << "[autostart-daemon] Missing executable for" << id << ":" << executable;
                continue;
            }

            if (!QProcess::startDetached(executable, {})) {
                qWarning() << "[autostart-daemon] Failed to launch" << executable;
                continue;
            }

            m_startedIds.insert(id);
            qInfo() << "[autostart-daemon] Started" << executable;
        }
    } catch (const std::exception &error) {
        qWarning() << "[autostart-daemon] Unable to process manifest:" << error.what();
    }

    if (desiredIds.isEmpty()) {
        m_startedIds.clear();
    } else {
        for (auto it = m_startedIds.begin(); it != m_startedIds.end();) {
            if (!desiredIds.contains(*it)) {
                it = m_startedIds.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void AutostartDaemon::onDisplayTimerTimeout()
{
    ensureDisplayReady();
}

bool AutostartDaemon::ensureDisplayReady()
{
    if (m_displayReady) {
        return true;
    }

    if (!refreshDisplayEnvironment()) {
        if (!m_displayTimer.isActive()) {
            m_displayTimer.start();
        }
        return false;
    }

    m_displayReady = true;
    if (m_displayTimer.isActive()) {
        m_displayTimer.stop();
    }

    const QString display = qEnvironmentVariable("DISPLAY");
    const QString wayland = qEnvironmentVariable("WAYLAND_DISPLAY");
    if (!display.isEmpty()) {
        qInfo().noquote().nospace() << "[autostart-daemon] display ready (DISPLAY=" << display << ')';
    } else if (!wayland.isEmpty()) {
        qInfo().noquote().nospace() << "[autostart-daemon] display ready (WAYLAND_DISPLAY=" << wayland << ')';
    } else {
        qInfo() << "[autostart-daemon] display detected";
    }

    if (m_pendingSync) {
        m_pendingSync = false;
        if (!m_syncTimer.isActive()) {
            m_syncTimer.start();
        }
    }

    return true;
}

bool AutostartDaemon::refreshDisplayEnvironment()
{
    QString display = qEnvironmentVariable("DISPLAY");
    QString wayland = qEnvironmentVariable("WAYLAND_DISPLAY");

    const EnvironmentMap env = readSystemdUserEnvironment();
    if (!env.isEmpty()) {
        for (auto it = env.constBegin(); it != env.constEnd(); ++it) {
            applyEnvironmentVariable(it.key(), it.value());
        }

        if (display.isEmpty()) {
            display = env.value(QStringLiteral("DISPLAY"));
        }
        if (wayland.isEmpty()) {
            wayland = env.value(QStringLiteral("WAYLAND_DISPLAY"));
        }
    }

    if (display.isEmpty()) {
        const QString candidate = probeX11Display();
        if (!candidate.isEmpty()) {
            applyEnvironmentVariable(QStringLiteral("DISPLAY"), candidate);
            display = candidate;
        }
    }

    if (wayland.isEmpty()) {
        const QString candidate = probeWaylandDisplay();
        if (!candidate.isEmpty()) {
            applyEnvironmentVariable(QStringLiteral("WAYLAND_DISPLAY"), candidate);
            wayland = candidate;
        }
    }

    return !display.isEmpty() || !wayland.isEmpty();
}

void AutostartDaemon::applyEnvironmentVariable(const QString &key, const QString &value)
{
    if (key.isEmpty()) {
        return;
    }

    const QByteArray keyUtf8 = key.toLocal8Bit();

    if (value.isNull() || value.isEmpty()) {
        qunsetenv(keyUtf8.constData());
        m_environmentCache.remove(key);
        return;
    }

    QByteArray &buffer = m_environmentCache[key];
    buffer = value.toUtf8();
    qputenv(keyUtf8.constData(), buffer);
}

void AutostartDaemon::updateManifestWatch()
{
    if (m_manifestPath.isEmpty()) {
        return;
    }

    const bool exists = QFileInfo::exists(m_manifestPath);
    const bool watchingFile = m_watcher.files().contains(m_manifestPath);

    if (exists && !watchingFile) {
        m_watcher.addPath(m_manifestPath);
    } else if (!exists && watchingFile) {
        m_watcher.removePath(m_manifestPath);
    }
}

} // namespace appimagemanager
