# AppImage 管理器

[View English README](../README.md)

AppImage 管理器是一个基于 Qt 的桌面与命令行应用，用于在 Linux 系统上集中管理 AppImage 包。图形界面提供现代化的操作体验，同时也保留了一套命令行工具，方便注册、启动和移除位于专用存储目录中的 AppImage。

## 功能

- 现代化的 Qt 窗口界面，可在列表与网格布局之间切换浏览已托管的 AppImage。
- 首次通过管理器打开 AppImage 时，会自动提示将其纳入托管。
- 默认将 AppImage 移动到 `~/.local/share/appimagemanager/apps` 的专属目录中统一管理。
- 支持自定义显示名称、单个应用的开机自启动，以及在列表或网格中直接启动。
- 提供首选项面板，可调整托管行为、删除确认、布局方式以及界面语言（英文/简体中文）。
- 持久化的清单文件记录每个 AppImage 的元数据。
- 附带命令行工具，便于自动化或脚本集成。
- 当 AppImage 未提供图标时，会自动生成首字母头像。
- 安装后会放置 `.desktop` 启动器，可直接被 Rofi、GNOME Shell 等启动器检索。

## 本地化

所有翻译都存放在 `resources/i18n` 下的 JSON 文件中。要新增或修改翻译，仅需更新对应的 JSON 文件并重新构建项目，Qt 会自动嵌入最新的资源。

## 安装依赖

项目依赖 Qt Widgets（Qt 5 或 Qt 6）。在配置 CMake 之前，请先安装相应的开发包和工具。

```bash
# Ubuntu / Debian（以 Qt 5 为例）
sudo apt update
sudo apt install qtbase5-dev qttools5-dev-tools

# Arch Linux（以 Qt 6 为例）
sudo pacman -S qt6-base
```

## 构建步骤

```bash
cmake -S . -B build
cmake --build build
sudo cmake --install build
```

安装步骤会把 `appimagemanager` 二进制放置到系统的 `${CMAKE_INSTALL_BINDIR}` 目录（通常是 `/usr/local/bin`）。

## 命令行用法

```text
appimagemanager                # 启动图形界面
appimagemanager add <path>     # 添加 AppImage 并移动到托管目录
appimagemanager remove <id>    # 移除一个托管中的 AppImage
appimagemanager list           # 列出所有托管中的 AppImage
appimagemanager autostart <id> <on|off>  # 打开或关闭指定 AppImage 的开机自启动
appimagemanager open <target>  # 通过 id 或路径打开 AppImage（未托管时会提示加入）
appimagemanager storage-dir    # 打印专用存储目录
appimagemanager manifest       # 打印清单文件路径
```

通过 `appimagemanager open` 启动 AppImage 时，如果目标尚未托管，管理器会提示是否纳入管理并移动到专用目录。
