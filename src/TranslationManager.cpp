#include "AppImageManager/TranslationManager.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QString>
#include <QStringList>
#include <QTranslator>

namespace appimagemanager {

namespace {

class JsonTranslator : public QTranslator {
public:
    JsonTranslator() = default;

    bool loadFromJson(const QByteArray &bytes)
    {
        QJsonParseError parseError{};
        const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            return false;
        }

        const QJsonObject root = document.object();
        for (auto contextIt = root.begin(); contextIt != root.end(); ++contextIt) {
            const QString context = contextIt.key();
            const QJsonValue contextValue = contextIt.value();
            if (!contextValue.isObject()) {
                continue;
            }
            const QJsonObject messages = contextValue.toObject();
            for (auto messageIt = messages.begin(); messageIt != messages.end(); ++messageIt) {
                const QString source = messageIt.key();
                const QJsonValue translationValue = messageIt.value();
                if (!translationValue.isString()) {
                    continue;
                }
                const QString key = makeKey(context, source);
                m_catalog.insert(key, translationValue.toString());
            }
        }

        return true;
    }

    QString translate(const char *context, const char *sourceText, const char *disambiguation, int n) const override
    {
        Q_UNUSED(disambiguation);
        Q_UNUSED(n);
        if (!context || !sourceText) {
            return QString();
        }

        const QString key = makeKey(QString::fromLatin1(context), QString::fromUtf8(sourceText));
        const auto it = m_catalog.constFind(key);
        if (it != m_catalog.constEnd()) {
            return it.value();
        }
        return QString();
    }

    bool isEmpty() const override
    {
        return m_catalog.isEmpty();
    }

private:
    static QString makeKey(const QString &context, const QString &source)
    {
        return context + QLatin1Char('\x1f') + source;
    }

private:
    QHash<QString, QString> m_catalog;
};

std::unique_ptr<QTranslator> createChineseSimplifiedTranslator()
{
    static const QStringList kCandidates = {
        QStringLiteral(":/i18n/zh_CN.json"),
        QStringLiteral(":/i18n/i18n/zh_CN.json"),
        QStringLiteral(":/zh_CN.json")
    };

    QByteArray bytes;
    QString sourcePath;
    for (const QString &candidate : kCandidates) {
        QFile file(candidate);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        const QByteArray data = file.readAll();
        if (data.isEmpty()) {
            continue;
        }
        bytes = data;
        sourcePath = candidate;
        break;
    }

    if (bytes.isEmpty()) {
        qWarning() << "[translator] unable to locate zh_CN.json in resources";
        return nullptr;
    }

    auto translator = std::make_unique<JsonTranslator>();
    if (!translator->loadFromJson(bytes)) {
        qWarning() << "[translator] failed to parse translation file at" << sourcePath;
        return nullptr;
    }

    if (translator->isEmpty()) {
        qWarning() << "[translator] translation catalog is empty after loading" << sourcePath;
        return nullptr;
    }

    return translator;
}

} // namespace

TranslationManager::TranslationManager()
    : m_selectedLanguage(LanguageOption::System)
    , m_activeLanguage(LanguageOption::English)
{
}

bool TranslationManager::applyLanguage(LanguageOption language)
{
    m_selectedLanguage = language;
    const LanguageOption effective = resolveEffectiveLanguage(language);

    if (m_translator) {
        qApp->removeTranslator(m_translator.get());
        m_translator.reset();
    }

    LanguageOption nextLanguage = effective;
    std::unique_ptr<QTranslator> translator;
    if (effective != LanguageOption::English) {
        translator = createTranslator(effective);
        if (!translator) {
            qWarning() << "[translator] falling back to English because the requested language could not be loaded";
            nextLanguage = LanguageOption::English;
        }
    }

    if (nextLanguage != LanguageOption::English && translator) {
        qApp->installTranslator(translator.get());
        m_translator = std::move(translator);
    }

    const bool changed = (nextLanguage != m_activeLanguage);
    m_activeLanguage = nextLanguage;
    return changed;
}

LanguageOption TranslationManager::resolveEffectiveLanguage(LanguageOption language) const
{
    if (language == LanguageOption::System) {
        const QLocale locale = QLocale::system();
        switch (locale.language()) {
        case QLocale::Chinese:
            return LanguageOption::ChineseSimplified;
        default:
            return LanguageOption::English;
        }
    }
    return language;
}

std::unique_ptr<QTranslator> TranslationManager::createTranslator(LanguageOption language) const
{
    if (language == LanguageOption::ChineseSimplified) {
        return createChineseSimplifiedTranslator();
    }
    return nullptr;
}

} // namespace appimagemanager
