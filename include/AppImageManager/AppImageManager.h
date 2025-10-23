#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace appimagelauncher {

struct AppImageEntry {
    std::string id;
    std::string name;
    std::filesystem::path storedPath;
    std::filesystem::path originalPath;
    bool autostart = false;
};

class AppImageManager {
public:
    AppImageManager();
    explicit AppImageManager(std::filesystem::path baseDirectory);

    void load();
    void save() const;

    const std::filesystem::path &baseDirectory() const noexcept;
    const std::filesystem::path &storageDirectory() const noexcept;

    std::vector<AppImageEntry> entries() const;

    std::optional<AppImageEntry> entryById(const std::string &id) const;
    std::optional<AppImageEntry> entryByStoredPath(const std::filesystem::path &path) const;
    std::optional<AppImageEntry> entryByOriginalPath(const std::filesystem::path &path) const;

    AppImageEntry addAppImage(const std::filesystem::path &path, bool moveToStorage = true);
    void removeAppImage(const std::string &id);
    void renameAppImage(const std::string &id, const std::string &displayName);

    bool isAutostartEnabled(const std::string &id) const;
    void setAutostart(const std::string &id, bool enabled);

    std::filesystem::path autostartDirectory() const noexcept;

    std::filesystem::path manifestPath() const;

private:
    std::filesystem::path ensureBaseDirectory(std::filesystem::path baseDirectory);
    void ensureStorageDirectory();
    std::filesystem::path ensureAutostartDirectory(std::filesystem::path directory) const;
    std::string generateId(const std::filesystem::path &path) const;
    std::filesystem::path autostartDesktopPath(const std::string &id) const;
    void writeAutostartEntry(const AppImageEntry &entry) const;
    void removeAutostartEntry(const std::string &id) const;

private:
    std::filesystem::path m_baseDirectory;
    std::filesystem::path m_storageDirectory;
    std::filesystem::path m_manifestPath;
    std::filesystem::path m_autostartDirectory;
    std::map<std::string, AppImageEntry> m_entries;
};

} // namespace appimagelauncher
