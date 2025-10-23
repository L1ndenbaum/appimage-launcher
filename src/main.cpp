#include "AppImageManager/AppImageManager.h"
#include "AppImageManager/MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QObject>
#include <QProcess>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using appimagelauncher::AppImageEntry;
using appimagelauncher::AppImageManager;
using appimagelauncher::MainWindow;

namespace {

void printUsage()
{
    std::cout << "AppImage Manager\n"
              << "Usage:\n"
              << "  appimagemanager                # Launch the graphical interface\n"
              << "  appimagemanager add <path>     # Add an AppImage and move it under management\n"
              << "  appimagemanager remove <id>    # Remove a managed AppImage\n"
              << "  appimagemanager list           # List all managed AppImages\n"
              << "  appimagemanager open <target>  # Open AppImage by id or path (prompts when new)\n"
              << "  appimagemanager storage-dir    # Print the dedicated storage directory\n"
              << "  appimagemanager manifest       # Print the manifest file path\n";
}

int handleCliCommand(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc < 2) {
        printUsage();
        return 0;
    }

    const std::string command = argv[1];
    AppImageManager manager;

    try {
        if (command == "add") {
            if (argc < 3) {
                std::cerr << "Missing AppImage path for add command" << std::endl;
                return 1;
            }
            const std::filesystem::path path = argv[2];
            const auto entry = manager.addAppImage(path, true);
            std::cout << "Added AppImage: " << entry.id << " (" << entry.storedPath << ")" << std::endl;
            return 0;
        }

        if (command == "remove") {
            if (argc < 3) {
                std::cerr << "Missing AppImage id" << std::endl;
                return 1;
            }
            manager.removeAppImage(argv[2]);
            std::cout << "Removed AppImage: " << argv[2] << std::endl;
            return 0;
        }

        if (command == "list") {
            const auto entries = manager.entries();
            for (const auto &entry : entries) {
                std::cout << entry.id << "\t" << entry.name << "\t" << entry.storedPath << std::endl;
            }
            return 0;
        }

        if (command == "storage-dir") {
            std::cout << manager.storageDirectory() << std::endl;
            return 0;
        }

        if (command == "manifest") {
            std::cout << manager.manifestPath() << std::endl;
            return 0;
        }

        if (command == "open") {
            // Fallback to GUI-enabled handler for prompt support
            return -1;
        }

        if (command == "help") {
            printUsage();
            return 0;
        }

        std::cerr << "Unknown command: " << command << std::endl;
        printUsage();
        return 1;
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << std::endl;
        return 1;
    }
}

int handleOpenCommand(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (argc < 3) {
        std::cerr << "Missing AppImage identifier or path" << std::endl;
        return 1;
    }

    const std::string target = argv[2];
    AppImageManager manager;

    auto entry = manager.entryById(target);
    if (!entry.has_value()) {
        std::filesystem::path candidate(target);
        if (std::filesystem::exists(candidate)) {
            entry = manager.entryByStoredPath(candidate);
            if (!entry.has_value()) {
                entry = manager.entryByOriginalPath(candidate);
            }

            if (!entry.has_value()) {
                const auto response = QMessageBox::question(
                    nullptr,
                    QObject::tr("Add AppImage"),
                    QObject::tr("The AppImage '%1' is not managed yet. Do you want to add it now?\nIt will be moved to the managed storage folder.")
                        .arg(QString::fromStdString(candidate.filename().string())));

                if (response == QMessageBox::Yes) {
                    try {
                        entry = manager.addAppImage(candidate, true);
                    } catch (const std::exception &error) {
                        QMessageBox::critical(nullptr, QObject::tr("Unable to add"), QString::fromUtf8(error.what()));
                        return 1;
                    }
                } else {
                    if (!QProcess::startDetached(QString::fromStdString(std::filesystem::absolute(candidate).string()), {})) {
                        QMessageBox::warning(nullptr, QObject::tr("Launch failed"), QObject::tr("Unable to start the AppImage."));
                        return 1;
                    }
                    return 0;
                }
            }
        } else {
            std::cerr << "Unknown AppImage target: " << target << std::endl;
            return 1;
        }
    }

    if (!entry.has_value()) {
        std::cerr << "Unable to locate AppImage" << std::endl;
        return 1;
    }

    if (!QProcess::startDetached(QString::fromStdString(entry->storedPath.string()), {})) {
        QMessageBox::warning(nullptr, QObject::tr("Launch failed"), QObject::tr("Unable to start the AppImage."));
        return 1;
    }

    return 0;
}

int runGui(int argc, char *argv[])
{
    QApplication app(argc, argv);
    AppImageManager manager;
    MainWindow window(manager);
    window.show();
    return app.exec();
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc > 1) {
        const int cliResult = handleCliCommand(argc, argv);
        if (cliResult != -1) {
            return cliResult;
        }
        return handleOpenCommand(argc, argv);
    }

    return runGui(argc, argv);
}
