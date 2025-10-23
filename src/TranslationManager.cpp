#include "AppImageManager/TranslationManager.h"

#include <QApplication>
#include <QHash>
#include <QLocale>
#include <QString>
#include <QTranslator>

namespace appimagelauncher {

namespace {

class SimpleTranslator : public QTranslator {
public:
    SimpleTranslator() = default;

    void add(const char *context, const char *sourceText, const QString &translation)
    {
        if (!context || !sourceText) {
            return;
        }
        const QString key = makeKey(QString::fromLatin1(context), QString::fromUtf8(sourceText));
        m_catalog.insert(key, translation);
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
    auto translator = std::make_unique<SimpleTranslator>();

    translator->add("appimagelauncher::MainWindow", "AppImage Manager", QString::fromUtf8("AppImage 管理器"));
    translator->add("appimagelauncher::MainWindow", "Actions", QString::fromUtf8("操作"));
    translator->add("appimagelauncher::MainWindow", "File", QString::fromUtf8("文件"));
    translator->add("appimagelauncher::MainWindow", "View", QString::fromUtf8("视图"));
    translator->add("appimagelauncher::MainWindow", "Settings", QString::fromUtf8("设置"));
    translator->add("appimagelauncher::MainWindow", "Add", QString::fromUtf8("添加"));
    translator->add("appimagelauncher::MainWindow", "Add a new AppImage", QString::fromUtf8("添加新的 AppImage"));
    translator->add("appimagelauncher::MainWindow", "Open", QString::fromUtf8("打开"));
    translator->add("appimagelauncher::MainWindow", "Launch the selected AppImage", QString::fromUtf8("启动所选的 AppImage"));
    translator->add("appimagelauncher::MainWindow", "Rename", QString::fromUtf8("重命名"));
    translator->add("appimagelauncher::MainWindow", "Rename the selected AppImage", QString::fromUtf8("重命名所选的 AppImage"));
    translator->add("appimagelauncher::MainWindow", "Remove", QString::fromUtf8("移除"));
    translator->add("appimagelauncher::MainWindow", "Remove the selected AppImage", QString::fromUtf8("移除所选的 AppImage"));
    translator->add("appimagelauncher::MainWindow", "Enable Autostart", QString::fromUtf8("启用自启动"));
    translator->add("appimagelauncher::MainWindow", "Disable Autostart", QString::fromUtf8("禁用自启动"));
    translator->add("appimagelauncher::MainWindow", "Open Storage", QString::fromUtf8("打开存储目录"));
    translator->add("appimagelauncher::MainWindow", "Show the managed storage directory", QString::fromUtf8("打开托管存储目录"));
    translator->add("appimagelauncher::MainWindow", "Toggle autostart for the selected AppImage", QString::fromUtf8("切换所选 AppImage 的自启动"));
    translator->add("appimagelauncher::MainWindow", "Select AppImage", QString::fromUtf8("选择 AppImage"));
    translator->add("appimagelauncher::MainWindow", "AppImage Files (*.AppImage);;All Files (*)",
        QString::fromUtf8("AppImage 文件 (*.AppImage);;所有文件 (*)"));
    translator->add("appimagelauncher::MainWindow", "Unable to add AppImage", QString::fromUtf8("无法添加 AppImage"));
    translator->add("appimagelauncher::MainWindow", "Unable to remove", QString::fromUtf8("无法移除"));
    translator->add("appimagelauncher::MainWindow", "Remove AppImage", QString::fromUtf8("移除 AppImage"));
    translator->add("appimagelauncher::MainWindow",
        "Do you really want to remove the selected AppImage?",
        QString::fromUtf8("确定要移除所选的 AppImage 吗？"));
    translator->add("appimagelauncher::MainWindow", "Launch failed", QString::fromUtf8("启动失败"));
    translator->add("appimagelauncher::MainWindow",
        "Unable to locate the stored AppImage.",
        QString::fromUtf8("无法找到已存储的 AppImage。"));
    translator->add("appimagelauncher::MainWindow",
        "Unable to start the AppImage.",
        QString::fromUtf8("无法启动该 AppImage。"));
    translator->add("appimagelauncher::MainWindow", "Missing AppImage", QString::fromUtf8("缺少 AppImage"));
    translator->add("appimagelauncher::MainWindow",
        "Unable to locate the selected AppImage.",
        QString::fromUtf8("无法找到所选的 AppImage。"));
    translator->add("appimagelauncher::MainWindow",
        "Unable to update autostart",
        QString::fromUtf8("无法更新自启动设置"));
    translator->add("appimagelauncher::MainWindow", "Preferences", QString::fromUtf8("首选项"));
    translator->add("appimagelauncher::MainWindow", "Open application settings", QString::fromUtf8("打开应用程序设置"));
    translator->add("appimagelauncher::MainWindow", "Quit", QString::fromUtf8("退出"));
    translator->add("appimagelauncher::MainWindow", "Quit AppImage Manager", QString::fromUtf8("退出 AppImage 管理器"));
    translator->add("appimagelauncher::MainWindow", "List view", QString::fromUtf8("列表视图"));
    translator->add("appimagelauncher::MainWindow", "Grid view", QString::fromUtf8("网格视图"));
    translator->add("appimagelauncher::MainWindow", " (Autostart)", QString::fromUtf8("（自启动）"));
    translator->add("appimagelauncher::MainWindow", "%n AppImage(s) managed", QString::fromUtf8("已管理 %n 个 AppImage"));
    translator->add("appimagelauncher::MainWindow", "Rename AppImage", QString::fromUtf8("重命名 AppImage"));
    translator->add("appimagelauncher::MainWindow", "New name", QString::fromUtf8("新名称"));
    translator->add("appimagelauncher::MainWindow", "The name must not be empty.", QString::fromUtf8("名称不能为空。"));
    translator->add("appimagelauncher::MainWindow", "Unable to rename AppImage", QString::fromUtf8("无法重命名 AppImage"));

    translator->add("QObject",
        "The AppImage '%1' is not managed yet. Do you want to add it now?\nIt will be moved to the managed storage folder.",
        QString::fromUtf8("AppImage“%1”尚未被管理。现在要添加吗？\n它将被移动到托管存储目录。"));
    translator->add("QObject", "Add AppImage", QString::fromUtf8("添加 AppImage"));
    translator->add("QObject", "Unable to add", QString::fromUtf8("无法添加"));
    translator->add("QObject", "Unable to start the AppImage.", QString::fromUtf8("无法启动该 AppImage。"));
    translator->add("QObject", "Launch failed", QString::fromUtf8("启动失败"));

    translator->add("appimagelauncher::SettingsDialog", "Preferences", QString::fromUtf8("首选项"));
    translator->add("appimagelauncher::SettingsDialog", "General", QString::fromUtf8("常规"));
    translator->add("appimagelauncher::SettingsDialog", "Move AppImages into managed storage", QString::fromUtf8("将 AppImage 移动到托管存储"));
    translator->add("appimagelauncher::SettingsDialog", "Ask for confirmation before removing", QString::fromUtf8("删除前询问确认"));
    translator->add("appimagelauncher::SettingsDialog", "Layout", QString::fromUtf8("布局"));
    translator->add("appimagelauncher::SettingsDialog", "List view", QString::fromUtf8("列表视图"));
    translator->add("appimagelauncher::SettingsDialog", "Grid view", QString::fromUtf8("网格视图"));
    translator->add("appimagelauncher::SettingsDialog", "Language", QString::fromUtf8("语言"));
    translator->add("appimagelauncher::SettingsDialog", "System default", QString::fromUtf8("跟随系统"));
    translator->add("appimagelauncher::SettingsDialog", "English", QString::fromUtf8("英语"));
    translator->add("appimagelauncher::SettingsDialog", "Chinese (Simplified)", QString::fromUtf8("简体中文"));

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
    if (effective == m_activeLanguage) {
        return false;
    }

    if (m_translator) {
        qApp->removeTranslator(m_translator.get());
        m_translator.reset();
    }

    auto translator = createTranslator(effective);
    if (translator) {
        qApp->installTranslator(translator.get());
        m_translator = std::move(translator);
    }

    m_activeLanguage = effective;
    return true;
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

} // namespace appimagelauncher
