#include "AppImageManager/Preferences.h"

#include <QSettings>

namespace appimagelauncher {

namespace {

constexpr const char *kPreferencesGroup = "preferences";
constexpr const char *kMoveToStorageKey = "moveToStorageOnAdd";
constexpr const char *kConfirmRemovalKey = "confirmRemoval";
constexpr const char *kViewModeKey = "viewMode";
constexpr const char *kLanguageKey = "language";

ViewMode decodeViewMode(int value)
{
    switch (value) {
    case 0:
        return ViewMode::List;
    case 1:
        return ViewMode::Grid;
    default:
        return ViewMode::List;
    }
}

LanguageOption decodeLanguage(int value)
{
    switch (value) {
    case 0:
        return LanguageOption::System;
    case 1:
        return LanguageOption::English;
    case 2:
        return LanguageOption::ChineseSimplified;
    default:
        return LanguageOption::System;
    }
}

} // namespace

Preferences Preferences::load()
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kPreferencesGroup));

    Preferences prefs;
    prefs.moveToStorageOnAdd = settings.value(QString::fromLatin1(kMoveToStorageKey), true).toBool();
    prefs.confirmRemoval = settings.value(QString::fromLatin1(kConfirmRemovalKey), true).toBool();
    prefs.viewMode = decodeViewMode(settings.value(QString::fromLatin1(kViewModeKey), 0).toInt());
    prefs.language = decodeLanguage(settings.value(QString::fromLatin1(kLanguageKey), 0).toInt());

    settings.endGroup();
    return prefs;
}

void Preferences::save() const
{
    QSettings settings;
    settings.beginGroup(QString::fromLatin1(kPreferencesGroup));

    settings.setValue(QString::fromLatin1(kMoveToStorageKey), moveToStorageOnAdd);
    settings.setValue(QString::fromLatin1(kConfirmRemovalKey), confirmRemoval);
    settings.setValue(QString::fromLatin1(kViewModeKey), static_cast<int>(viewMode));
    settings.setValue(QString::fromLatin1(kLanguageKey), static_cast<int>(language));

    settings.endGroup();
    settings.sync();
}

} // namespace appimagelauncher
