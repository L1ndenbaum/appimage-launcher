#include "AppImageManager/AppImageManager.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace appimagemanager {

namespace {
std::filesystem::path defaultBaseDirectory()
{
    if (const char *xdgDataHome = std::getenv("XDG_DATA_HOME")) {
        if (*xdgDataHome != '\0') {
            return std::filesystem::path(xdgDataHome) / "appimagemanager";
        }
    }

    const char *home = std::getenv("HOME");
    if (!home || *home == '\0') {
        throw std::runtime_error("Unable to determine HOME directory for AppImageManager storage");
    }

    return std::filesystem::path(home) / ".local" / "share" / "appimagemanager";
}

std::filesystem::path defaultAutostartDirectory()
{
    if (const char *xdgConfigHome = std::getenv("XDG_CONFIG_HOME")) {
        if (*xdgConfigHome != '\0') {
            return std::filesystem::path(xdgConfigHome) / "autostart";
        }
    }

    const char *home = std::getenv("HOME");
    if (!home || *home == '\0') {
        throw std::runtime_error("Unable to determine HOME directory for AppImageManager autostart entries");
    }

    return std::filesystem::path(home) / ".config" / "autostart";
}

std::string sanitizeId(std::string base)
{
    for (char &ch : base) {
        if (!std::isalnum(static_cast<unsigned char>(ch))) {
            ch = '-';
        }
    }
    while (!base.empty() && base.front() == '-') {
        base.erase(base.begin());
    }
    while (!base.empty() && base.back() == '-') {
        base.pop_back();
    }
    if (base.empty()) {
        base = "appimage";
    }
    return base;
}

std::string splitStem(const std::filesystem::path &path)
{
    auto stem = path.stem().string();
    if (stem.empty()) {
        stem = path.filename().string();
    }
    return stem;
}

std::string trim(std::string value)
{
    auto isSpace = [](unsigned char ch) { return std::isspace(ch); };
    value.erase(value.begin(),
        std::find_if(value.begin(), value.end(), [&](char ch) { return !isSpace(static_cast<unsigned char>(ch)); }));
    value.erase(
        std::find_if(value.rbegin(), value.rend(), [&](char ch) { return !isSpace(static_cast<unsigned char>(ch)); })
            .base(),
        value.end());
    return value;
}

} // namespace

AppImageManager::AppImageManager()
    : AppImageManager(defaultBaseDirectory())
{
}

AppImageManager::AppImageManager(std::filesystem::path baseDirectory)
    : m_baseDirectory(ensureBaseDirectory(std::move(baseDirectory)))
    , m_storageDirectory(m_baseDirectory / "apps")
    , m_manifestPath(m_baseDirectory / "manifest.tsv")
    , m_autostartDirectory(ensureAutostartDirectory(defaultAutostartDirectory()))
{
    ensureStorageDirectory();
    load();
}

void AppImageManager::load()
{
    m_entries.clear();
    std::ifstream stream(m_manifestPath);
    if (!stream.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream lineStream(line);
        std::string id;
        std::string name;
        std::string storedPath;
        std::string originalPath;
        std::string autostartFlag;

        if (!std::getline(lineStream, id, '\t')) {
            continue;
        }
        if (!std::getline(lineStream, name, '\t')) {
            continue;
        }
        if (!std::getline(lineStream, storedPath, '\t')) {
            continue;
        }
        if (!std::getline(lineStream, originalPath, '\t')) {
            originalPath.clear();
        }
        if (!std::getline(lineStream, autostartFlag, '\t')) {
            autostartFlag.clear();
        }

        const bool autostart = autostartFlag == "1" || autostartFlag == "true" || autostartFlag == "yes";

        AppImageEntry entry{
            id,
            name,
            std::filesystem::path(storedPath),
            std::filesystem::path(originalPath),
            autostart
        };
        m_entries.emplace(entry.id, std::move(entry));
    }
}

void AppImageManager::save() const
{
    std::ofstream stream(m_manifestPath, std::ios::trunc);
    if (!stream.is_open()) {
        throw std::runtime_error("Unable to write AppImage manifest: " + m_manifestPath.string());
    }

    for (const auto &pair : m_entries) {
        const auto &entry = pair.second;
        stream << entry.id << '\t'
               << entry.name << '\t'
               << entry.storedPath.string() << '\t'
               << entry.originalPath.string() << '\t'
               << (entry.autostart ? "1" : "0") << '\n';
    }
}

const std::filesystem::path &AppImageManager::baseDirectory() const noexcept
{
    return m_baseDirectory;
}

const std::filesystem::path &AppImageManager::storageDirectory() const noexcept
{
    return m_storageDirectory;
}

std::vector<AppImageEntry> AppImageManager::entries() const
{
    std::vector<AppImageEntry> result;
    result.reserve(m_entries.size());
    for (const auto &pair : m_entries) {
        result.push_back(pair.second);
    }
    return result;
}

std::optional<AppImageEntry> AppImageManager::entryById(const std::string &id) const
{
    auto it = m_entries.find(id);
    if (it == m_entries.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<AppImageEntry> AppImageManager::entryByStoredPath(const std::filesystem::path &path) const
{
    try {
        const auto normalizedInput = std::filesystem::absolute(path);
        for (const auto &pair : m_entries) {
            const auto normalizedStored = std::filesystem::absolute(pair.second.storedPath);
            if (normalizedStored == normalizedInput) {
                return pair.second;
            }
        }
    } catch (const std::filesystem::filesystem_error &) {
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<AppImageEntry> AppImageManager::entryByOriginalPath(const std::filesystem::path &path) const
{
    try {
        const auto normalizedInput = std::filesystem::absolute(path);
        for (const auto &pair : m_entries) {
            if (pair.second.originalPath.empty()) {
                continue;
            }
            const auto normalizedOriginal = std::filesystem::absolute(pair.second.originalPath);
            if (normalizedOriginal == normalizedInput) {
                return pair.second;
            }
        }
    } catch (const std::filesystem::filesystem_error &) {
        return std::nullopt;
    }
    return std::nullopt;
}

AppImageEntry AppImageManager::addAppImage(const std::filesystem::path &path, bool moveToStorage)
{
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("AppImage does not exist: " + path.string());
    }

    std::filesystem::path absolutePath = std::filesystem::absolute(path);
    std::filesystem::path destination = m_storageDirectory / path.filename();

    std::string stem = splitStem(path);
    std::string extension = path.has_extension() ? path.extension().string() : std::string();
    int suffix = 1;
    while (std::filesystem::exists(destination)) {
        destination = m_storageDirectory / (stem + "-" + std::to_string(suffix++) + extension);
    }

    std::filesystem::path storedPath = destination;
    if (moveToStorage) {
        try {
            std::filesystem::rename(path, destination);
        } catch (const std::filesystem::filesystem_error &) {
            std::filesystem::copy_file(path, destination, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::remove(path);
        }
    } else {
        storedPath = absolutePath;
    }

    std::string id = generateId(storedPath);

    std::string displayName = trim(splitStem(storedPath));
    if (displayName.empty()) {
        displayName = storedPath.filename().string();
    }

    AppImageEntry entry{
        id,
        displayName,
        storedPath,
        moveToStorage ? absolutePath : std::filesystem::path{},
        false
    };
    m_entries[entry.id] = entry;
    save();
    return entry;
}

void AppImageManager::removeAppImage(const std::string &id)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end()) {
        throw std::runtime_error("Unknown AppImage id: " + id);
    }

    const auto storedPath = it->second.storedPath;
    removeAutostartEntry(id);
    m_entries.erase(it);
    save();

    try {
        if (!storedPath.empty() && std::filesystem::exists(storedPath)) {
            std::filesystem::remove(storedPath);
        }
    } catch (const std::filesystem::filesystem_error &) {
        // Ignore removal errors.
    }
}

void AppImageManager::renameAppImage(const std::string &id, const std::string &displayName)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end()) {
        throw std::runtime_error("Unknown AppImage id: " + id);
    }

    const std::string trimmedName = trim(displayName);
    if (trimmedName.empty()) {
        throw std::runtime_error("Display name must not be empty");
    }

    if (it->second.name == trimmedName) {
        return;
    }

    const std::string previousName = it->second.name;
    it->second.name = trimmedName;
    try {
        if (it->second.autostart) {
            writeAutostartEntry(it->second);
        }
        save();
    } catch (...) {
        it->second.name = previousName;
        throw;
    }
}

std::filesystem::path AppImageManager::manifestPath() const
{
    return m_manifestPath;
}

std::filesystem::path AppImageManager::ensureBaseDirectory(std::filesystem::path baseDirectory)
{
    if (baseDirectory.empty()) {
        baseDirectory = defaultBaseDirectory();
    }
    std::filesystem::create_directories(baseDirectory);
    return baseDirectory;
}

void AppImageManager::ensureStorageDirectory()
{
    std::filesystem::create_directories(m_storageDirectory);
}

std::filesystem::path AppImageManager::ensureAutostartDirectory(std::filesystem::path directory) const
{
    if (!directory.empty()) {
        std::filesystem::create_directories(directory);
    }
    return directory;
}

std::string AppImageManager::generateId(const std::filesystem::path &path) const
{
    std::string idBase = sanitizeId(splitStem(path));
    std::string id = idBase;
    int suffix = 1;
    while (m_entries.find(id) != m_entries.end()) {
        id = idBase + "-" + std::to_string(suffix++);
    }
    return id;
}

bool AppImageManager::isAutostartEnabled(const std::string &id) const
{
    auto it = m_entries.find(id);
    if (it == m_entries.end()) {
        throw std::runtime_error("Unknown AppImage id: " + id);
    }
    return it->second.autostart;
}

void AppImageManager::setAutostart(const std::string &id, bool enabled)
{
    auto it = m_entries.find(id);
    if (it == m_entries.end()) {
        throw std::runtime_error("Unknown AppImage id: " + id);
    }

    if (it->second.autostart == enabled) {
        return;
    }

    const bool previous = it->second.autostart;
    it->second.autostart = enabled;
    try {
        if (enabled) {
            writeAutostartEntry(it->second);
        } else {
            removeAutostartEntry(id);
        }
        save();
    } catch (...) {
        it->second.autostart = previous;
        throw;
    }
}

std::filesystem::path AppImageManager::autostartDirectory() const noexcept
{
    return m_autostartDirectory;
}

std::filesystem::path AppImageManager::autostartDesktopPath(const std::string &id) const
{
    return m_autostartDirectory / ("appimagemanager-" + id + ".desktop");
}

void AppImageManager::writeAutostartEntry(const AppImageEntry &entry) const
{
    if (m_autostartDirectory.empty()) {
        return;
    }

    std::filesystem::create_directories(m_autostartDirectory);
    const auto desktopPath = autostartDesktopPath(entry.id);
    std::ofstream stream(desktopPath, std::ios::trunc);
    if (!stream.is_open()) {
        throw std::runtime_error("Unable to write autostart entry: " + desktopPath.string());
    }

    const std::string execPath = entry.storedPath.string();
    std::string escapedExec;
    escapedExec.reserve(execPath.size());
    for (char ch : execPath) {
        if (ch == '"') {
            escapedExec += "\\\"";
        } else {
            escapedExec += ch;
        }
    }

    stream << "[Desktop Entry]\n"
           << "Type=Application\n"
           << "Name=" << entry.name << "\n"
           << "Exec=\"" << escapedExec << "\"\n"
           << "Terminal=false\n"
           << "X-AppImage-Id=" << entry.id << "\n";
    if (!entry.originalPath.empty()) {
        stream << "X-AppImage-Original-Path=" << entry.originalPath.string() << "\n";
    }
    stream << "X-GNOME-Autostart-enabled=true\n";
}

void AppImageManager::removeAutostartEntry(const std::string &id) const
{
    if (m_autostartDirectory.empty()) {
        return;
    }

    const auto desktopPath = autostartDesktopPath(id);
    try {
        if (std::filesystem::exists(desktopPath)) {
            std::filesystem::remove(desktopPath);
        }
    } catch (const std::filesystem::filesystem_error &) {
        // Ignore failures when removing autostart entries.
    }
}

} // namespace appimagemanager
