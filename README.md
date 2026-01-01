# UniSettings

A Qt-based unified settings library with file monitoring and cross-application synchronization for Uni desktop environment.

## Features

- **Unified Configuration Management**: Single API for both application-specific and system-wide settings
- **Automatic Change Detection**: File-system watcher with debounced change detection for real-time synchronization
- **Qt Native**: Built on QSettings with full support for Qt data types and QVariant
- **Hierarchical Organization**: Group-based key organization for clean configuration structure
- **Thread-Safe Singleton**: Mutex-protected singleton pattern for system-wide settings access
- **QML Integration**: SystemSettings provides Q_INVOKABLE methods for QML access
- **Macro-Based Property System**: Declarative property definitions with automatic signal emission
- **Cross-Application Reading**: Read settings from other applications within the same scope

## Building

### Requirements

- Qt6 Core
- C++20 compiler
- CMake 3.16+

### Build Instructions

```bash
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install . --prefix=/usr
```

The library installs headers to `/usr/include/unisettings/` and the shared library to the system library directory.

## Core API

### UniSettings Class

The main settings class supporting both system-wide and application-specific configurations.

#### Basic Usage

```cpp
#include <unisettings.h>

// Application-specific settings
auto *settings = new UniSettings("myapp", this);

// Write values
settings->setValue("window/width", 800);
settings->setValue("theme", "dark"); // specific to app

// Read with defaults
int width = settings->value("window/width", 1024).toInt();
QString theme = settings->value("theme", "light").toString();

// Force synchronization if smth goes wrong
settings->sync();
```

#### System-Wide Settings

```cpp
// Access system singleton
auto *systemSettings = UniSettings::instance();
systemSettings->setValue("global/theme", "dark");

// Read system settings from app scope
QVariant theme = settings->systemValue("global/theme", "light"); // fallback system theme is light
```

#### Hierarchical Groups

```cpp
settings->beginGroup("window");
settings->setValue("width", 1920);    // Stored as "window/width"
settings->setValue("height", 1080);   // Stored as "window/height"
settings->endGroup();

QString currentGroup = settings->group();
```

#### Change Notifications

```cpp
// Local value changes
connect(settings, &UniSettings::valueChanged,
        [](const QString &key, const QVariant &value) {
    qDebug() << "Changed:" << key << "=" << value;
});

// External file changes (other processes)
connect(settings, &UniSettings::externalValueChanged,
        [](const QString &appName, const QString &key, const QVariant &value) {
    qDebug() << appName << "setting" << key << "changed to" << value;
});
```

#### Cross-Application Access

```cpp
// Read another app's settings
QVariant otherValue = settings->appValue("otherapp", "some/key", "fallback");

// Check key existence
if (settings->contains("theme")) {
    ...
}

// List all keys
QStringList keys = settings->allKeys();

// Remove keys
settings->remove("obsolete/setting");

// Clear all settings
settings->clear();
```

### SystemSettings Class

QML-friendly singleton wrapper around UniSettings for system-wide configuration.

```cpp
#include <systemsettings.h>

auto *sys = SystemSettings::instance();

// Basic operations
sys->setValue("theme", "dark");
QVariant theme = sys->value("theme", "light");

// Read application settings from system scope
QVariant appTheme = sys->appValue("myapp", "theme", "dark");

// Signals
connect(sys, &SystemSettings::settingChanged,
        [](const QString &key, const QVariant &value) {
    qDebug() << "System setting changed:" << key;
});
```

### Macro-Based Property System

The macro system provides declarative property definitions with automatic signal handling [file:2].

#### Defining a Settings Class

```cpp
#include <unisettings_macros.h>

// Define settings class
UNISETTINGS_CLASS(AppSettings, "myapp")
    // Property with explicit key
    UNISETTINGS_PROPERTY(QString, theme, "ui/theme", "light")

    // Property with auto-generated key (uses property name)
    UNISETTINGS_PROPERTY_AUTO(int, windowWidth, 1024)

    // Grouped property
    UNISETTINGS_PROPERTY_GROUP(bool, maximized, "window", "maximized", false)
UNISETTINGS_END()
```

#### Implementation

```cpp
// Implement loading and change handling
UNISETTINGS_IMPL_BEGIN(AppSettings)
    UNISETTINGS_LOAD_PROPERTY(theme)
    UNISETTINGS_LOAD_PROPERTY(windowWidth)
    UNISETTINGS_LOAD_PROPERTY(maximized)
UNISETTINGS_IMPL_CHANGE_BEGIN(AppSettings)
    UNISETTINGS_HANDLE_CHANGE("ui/theme", theme)
    UNISETTINGS_HANDLE_CHANGE("windowWidth", windowWidth)
    UNISETTINGS_HANDLE_CHANGE("maximized", maximized)
UNISETTINGS_IMPL_END()
```

#### Using the Settings Class

```cpp
auto *appSettings = new AppSettings(this);

// Access as properties
QString currentTheme = appSettings->theme();
appSettings->setTheme("dark");

// Listen for changes
connect(appSettings, &AppSettings::themeChanged, []() {
    qDebug() << "Theme changed!";
});

// Access underlying UniSettings object
UniSettings *raw = appSettings->settings();
```

## Architecture

### File Storage

Settings are stored as INI files in the XDG config directory:

- **System settings**: `~/.config/unisettings/system.conf`
- **Application settings**: `~/.config/unisettings/<appname>.conf`

### Change Detection

The library uses `QFileSystemWatcher` to monitor configuration files and directories. Changes are debounced with a 100ms timer to prevent excessive signal emission during bulk updates. An internal cache tracks values to detect actual changes versus file-system noise.

### Scope System

Two scopes are supported:

- **SystemScope**: Desktop environment settings (system.conf)
- **ApplicationScope**: Per-application settings (<appname>.conf)

The system scope singleton watches all `.conf` files in the settings directory and emits `externalValueChanged` signals when any application's settings change.

## API Reference

### UniSettings Methods

| Method | Description |
|--------|-------------|
| `value(key, default)` | Read setting with default fallback |
| `setValue(key, value)` | Write setting and emit signals |
| `contains(key)` | Check if key exists |
| `remove(key)` | Delete a setting |
| `allKeys()` | List all keys in current scope |
| `clear()` | Remove all settings |
| `sync()` | Force write to disk |
| `beginGroup(prefix)` | Start hierarchical group |
| `endGroup()` | End current group |
| `group()` | Get current group path |
| `systemValue(key, default)` | Read from system scope |
| `appValue(app, key, default)` | Read from another app |
| `applicationName()` | Get application name |
| `scope()` | Get current scope |

### Signals

- `valueChanged(QString key, QVariant value)` - Emitted on local changes
- `externalValueChanged(QString appName, QString key, QVariant value)` - Emitted on file changes

## License

MIT
