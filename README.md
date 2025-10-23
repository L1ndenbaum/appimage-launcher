# AppImage Manager

[阅读中文版 README](docs/README.zh_CN.md)

A Qt-based desktop and command-line application for managing AppImage packages on Linux systems. The manager provides a modern graphical interface along with a set of CLI tools for registering, launching, and removing AppImages from a dedicated storage directory.

## Features

- Modern Qt Widgets interface with list and grid layouts for browsing managed AppImages.
- Automatic prompt to register AppImages the first time they are opened through the manager.
- Moves managed AppImages to an exclusive storage directory under `~/.local/share/appimagemanager/apps` by default.
- Supports friendly renaming, per-application autostart, and quick launch directly from the grid or list.
- Preferences dialog for toggling storage behaviour, removal confirmations, layout, and language (English or Simplified Chinese).
- Persistent manifest tracking metadata for each AppImage.
- Command-line operations for automation and scripting.
- Automatically generates avatars when AppImages do not ship icons.
- Installs a `.desktop` launcher so the manager is discoverable via desktop launchers such as Rofi or GNOME Shell.

## Localization

Translations live in JSON catalogs under `resources/i18n`. Additions or edits only require updating the corresponding JSON file and rebuilding so Qt can embed the updated resources.

## Install Dependencies

This project depends on Qt Widgets (Qt 5 or Qt 6). Ensure the development headers and tools are available before configuring CMake.

```bash
# Ubuntu / Debian (Qt 5 example)
sudo apt update
sudo apt install qtbase5-dev qttools5-dev-tools

# Arch Linux (Qt 6 example)
sudo pacman -S qt6-base
```

## Building

```bash
cmake -S . -B build
cmake --build build
sudo cmake --install build
```

The install step places the `appimagemanager` binary in the system's `${CMAKE_INSTALL_BINDIR}` (typically `/usr/local/bin`).

## Command-line Usage

```text
appimagemanager                # Launch the graphical interface
appimagemanager add <path>     # Add an AppImage and move it under management
appimagemanager remove <id>    # Remove a managed AppImage
appimagemanager list           # List all managed AppImages
appimagemanager autostart <id> <on|off>  # Enable or disable login autostart for an AppImage
appimagemanager open <target>  # Open AppImage by id or path (prompts when new)
appimagemanager storage-dir    # Print the dedicated storage directory
appimagemanager manifest       # Print the manifest file path
```

Launching AppImages through `appimagemanager open` ensures that untracked packages prompt for registration and are moved to the managed storage folder when approved.
