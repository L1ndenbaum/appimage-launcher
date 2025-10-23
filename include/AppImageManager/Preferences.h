#pragma once

#include <QtGlobal>

namespace appimagelauncher {

enum class ViewMode {
    List,
    Grid
};

enum class LanguageOption {
    System,
    English,
    ChineseSimplified
};

struct Preferences {
    bool moveToStorageOnAdd = true;
    bool confirmRemoval = true;
    ViewMode viewMode = ViewMode::List;
    LanguageOption language = LanguageOption::System;

    static Preferences load();
    void save() const;
};

} // namespace appimagelauncher
