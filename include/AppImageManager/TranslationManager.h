#pragma once

#include "AppImageManager/Preferences.h"

#include <QTranslator>
#include <memory>

namespace appimagelauncher {

class TranslationManager {
public:
    TranslationManager();

    LanguageOption selectedLanguage() const noexcept { return m_selectedLanguage; }
    LanguageOption activeLanguage() const noexcept { return m_activeLanguage; }

    bool applyLanguage(LanguageOption language);

private:
    LanguageOption resolveEffectiveLanguage(LanguageOption language) const;
    std::unique_ptr<QTranslator> createTranslator(LanguageOption language) const;

private:
    LanguageOption m_selectedLanguage;
    LanguageOption m_activeLanguage;
    std::unique_ptr<QTranslator> m_translator;
};

} // namespace appimagelauncher
