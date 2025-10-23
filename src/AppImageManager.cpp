#include "AppImageManager/AppImageManager.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace appimagelauncher {

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

} // namespace

AppImageManager::AppImageManager()
    : AppImageManager(defaultBaseDirectory())
{
}

AppImageManager::AppImageManager(std::filesystem::path baseDirectory)
    : m_baseDirectory(ensureBaseDirectory(std::move(baseDirectory)))
    , m_storageDirectory(m_baseDirectory / "apps")
    , m_manifestPath(m_baseDirectory / "manifest.tsv")
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

        AppImageEntry entry{
            id,
            name,
            std::filesystem::path(storedPath),
            std::filesystem::path(originalPath)
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
               << entry.originalPath.string() << '\n';
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

    AppImageEntry entry{
        id,
        storedPath.filename().string(),
        storedPath,
        moveToStorage ? absolutePath : std::filesystem::path{}
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

} // namespace appimagelauncher
