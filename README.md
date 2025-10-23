# AppImage Manager

A Qt-based desktop and command-line application for managing AppImage packages on Linux systems. The manager provides a modern graphical interface along with a set of CLI tools for registering, launching, and removing AppImages from a dedicated storage directory.

## Features

- Modern Qt Widgets interface for browsing and launching managed AppImages.
- Automatic prompt to register AppImages the first time they are opened through the manager.
- Moves managed AppImages to an exclusive storage directory under `~/.local/share/appimagemanager/apps` by default.
- Persistent manifest tracking metadata for each AppImage.
- Command-line operations for automation and scripting.
- Optional per-application autostart entries for launching critical AppImages when the user logs in.

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
