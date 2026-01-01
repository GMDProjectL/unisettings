#include "unisettings.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QHash>

class UniSettingsPrivate
{
public:
    QString appName;
    UniSettings::Scope scope;
    QString configPath;
    QSettings *settings;
    QString currentGroup;
    QFileSystemWatcher *watcher;
    QTimer *debounceTimer;
    QHash<QString, QVariant> cachedValues;
    // Track cached values for all apps (system scope only)
    QHash<QString, QHash<QString, QVariant>> appCachedValues;
    bool ignoreNextChange;

    UniSettingsPrivate(const QString &app, UniSettings::Scope s)
        : appName(app)
        , scope(s)
        , settings(nullptr)
        , watcher(nullptr)
        , debounceTimer(nullptr)
        , ignoreNextChange(false)
    {
        QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        configDir += "/unisettings";
        QDir dir;
        dir.mkpath(configDir);
        if (scope == UniSettings::SystemScope) {
            configPath = configDir + "/system.conf";
        } else {
            configPath = configDir + "/" + appName + ".conf";
        }
        settings = new QSettings(configPath, QSettings::IniFormat);
        cacheAllValues();
    }

    ~UniSettingsPrivate()
    {
        delete settings;
        delete watcher;
        delete debounceTimer;
    }

    QString fullKey(const QString &key) const
    {
        if (currentGroup.isEmpty()) {
            return key;
        }
        return currentGroup + "/" + key;
    }

    void cacheAllValues()
    {
        cachedValues.clear();
        QStringList keys = settings->allKeys();
        for (const QString &key : keys) {
            cachedValues[key] = settings->value(key);
        }
    }

    QHash<QString, QVariant> detectChanges()
    {
        QHash<QString, QVariant> changes;
        if (ignoreNextChange) {
            ignoreNextChange = false;
            return changes;
        }

        settings->sync();
        QStringList currentKeys = settings->allKeys();
        QSet<QString> currentKeySet(currentKeys.begin(), currentKeys.end());
        QStringList cachedKeysList = cachedValues.keys();
        QSet<QString> cachedKeySet(cachedKeysList.begin(), cachedKeysList.end());

        for (const QString &key : currentKeys) {
            QVariant newValue = settings->value(key);
            QVariant oldValue = cachedValues.value(key);
            if (!cachedValues.contains(key) || newValue != oldValue) {
                changes[key] = newValue;
                cachedValues[key] = newValue;
            }
        }

        QSet<QString> removedKeys = cachedKeySet - currentKeySet;
        for (const QString &key : removedKeys) {
            changes[key] = QVariant();
            cachedValues.remove(key);
        }

        return changes;
    }

    // Detect changes for a specific app config (system scope only)
    QHash<QString, QVariant> detectAppChanges(const QString &appConfigPath, const QString &appName)
    {
        QHash<QString, QVariant> changes;
        QSettings appSettings(appConfigPath, QSettings::IniFormat);
        
        QStringList currentKeys = appSettings.allKeys();
        QSet<QString> currentKeySet(currentKeys.begin(), currentKeys.end());
        
        QHash<QString, QVariant> &cached = appCachedValues[appName];
        QStringList cachedKeysList = cached.keys();
        QSet<QString> cachedKeySet(cachedKeysList.begin(), cachedKeysList.end());

        for (const QString &key : currentKeys) {
            QVariant newValue = appSettings.value(key);
            QVariant oldValue = cached.value(key);
            if (!cached.contains(key) || newValue != oldValue) {
                changes[key] = newValue;
                cached[key] = newValue;
            }
        }

        QSet<QString> removedKeys = cachedKeySet - currentKeySet;
        for (const QString &key : removedKeys) {
            changes[key] = QVariant();
            cached.remove(key);
        }

        return changes;
    }
};

static UniSettings *s_instance = nullptr;
static QMutex s_mutex;

UniSettings* UniSettings::instance()
{
    if (!s_instance) {
        QMutexLocker locker(&s_mutex);
        if (!s_instance) {
            s_instance = new UniSettings(nullptr);
        }
    }
    return s_instance;
}

UniSettings::UniSettings(QObject *parent)
    : QObject(parent)
    , d_ptr(new UniSettingsPrivate("system", SystemScope))
{
    Q_D(UniSettings);
    
    d->watcher = new QFileSystemWatcher(this);
    QFileInfo fileInfo(d->configPath);
    
    // Watch system.conf if it exists
    if (fileInfo.exists()) {
        if (!d->watcher->addPath(d->configPath)) {
            qWarning() << "Failed to watch config file:" << d->configPath;
        }
    }

    QString configDir = fileInfo.absolutePath();
    if (QFileInfo(configDir).exists()) {
        d->watcher->addPath(configDir);
        
        // Watch all existing .conf files for system scope
        QDir dir(configDir);
        QStringList filters;
        filters << "*.conf";
        QFileInfoList configFiles = dir.entryInfoList(filters, QDir::Files);
        for (const QFileInfo &configFile : configFiles) {
            QString filePath = configFile.absoluteFilePath();
            if (filePath != d->configPath && !d->watcher->files().contains(filePath)) {
                d->watcher->addPath(filePath);
                // Initialize cache for this app
                QString appName = configFile.baseName();
                d->detectAppChanges(filePath, appName);
            }
        }
    }

    d->debounceTimer = new QTimer(this);
    d->debounceTimer->setSingleShot(true);
    d->debounceTimer->setInterval(100);

    connect(d->watcher, &QFileSystemWatcher::fileChanged,
            this, &UniSettings::onFileChanged);
    connect(d->watcher, &QFileSystemWatcher::directoryChanged,
            this, &UniSettings::onFileChanged);

    connect(d->debounceTimer, &QTimer::timeout, this, [this]() {
        Q_D(UniSettings);
        
        // Check system.conf changes
        QHash<QString, QVariant> changes = d->detectChanges();
        for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
            emit externalValueChanged("system", it.key(), it.value());
        }

        // Check all app configs in directory
        QString configDir = QFileInfo(d->configPath).absolutePath();
        QDir dir(configDir);
        QStringList filters;
        filters << "*.conf";
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
        
        for (const QFileInfo &fileInfo : files) {
            QString appName = fileInfo.baseName();
            QString filePath = fileInfo.absoluteFilePath();
            
            if (appName != "system") {
                QHash<QString, QVariant> appChanges = d->detectAppChanges(filePath, appName);
                for (auto it = appChanges.constBegin(); it != appChanges.constEnd(); ++it) {
                    emit externalValueChanged(appName, it.key(), it.value());
                }
                
                // Add to watcher if not already watched
                if (!d->watcher->files().contains(filePath)) {
                    d->watcher->addPath(filePath);
                }
            }
        }

        // Re-add system.conf if removed
        if (!d->watcher->files().contains(d->configPath)) {
            QFileInfo fileInfo(d->configPath);
            if (fileInfo.exists()) {
                d->watcher->addPath(d->configPath);
            }
        }
    });
}

UniSettings::UniSettings(const QString &appName, QObject *parent)
    : QObject(parent)
    , d_ptr(new UniSettingsPrivate(appName, ApplicationScope))
{
    Q_D(UniSettings);
    
    d->watcher = new QFileSystemWatcher(this);
    QFileInfo fileInfo(d->configPath);
    
    if (fileInfo.exists()) {
        if (!d->watcher->addPath(d->configPath)) {
            qWarning() << "Failed to watch config file:" << d->configPath;
        }
    }

    QString configDir = fileInfo.absolutePath();
    if (QFileInfo(configDir).exists()) {
        d->watcher->addPath(configDir);
    }

    d->debounceTimer = new QTimer(this);
    d->debounceTimer->setSingleShot(true);
    d->debounceTimer->setInterval(100);

    connect(d->watcher, &QFileSystemWatcher::fileChanged,
            this, &UniSettings::onFileChanged);
    connect(d->watcher, &QFileSystemWatcher::directoryChanged,
            this, &UniSettings::onFileChanged);

    connect(d->debounceTimer, &QTimer::timeout, this, [this]() {
        Q_D(UniSettings);
        QHash<QString, QVariant> changes = d->detectChanges();
        for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
            emit valueChanged(it.key(), it.value());  // Added for local monitoring
            emit externalValueChanged(d->appName, it.key(), it.value());
        }

        if (!d->watcher->files().contains(d->configPath)) {
            QFileInfo fileInfo(d->configPath);
            if (fileInfo.exists()) {
                d->watcher->addPath(d->configPath);
            }
        }
    });
}

UniSettings::~UniSettings()
{
}

QVariant UniSettings::value(const QString &key, const QVariant &defaultValue) const
{
    Q_D(const UniSettings);
    return d->settings->value(d->fullKey(key), defaultValue);
}

void UniSettings::setValue(const QString &key, const QVariant &value)
{
    Q_D(UniSettings);
    QString fullKey = d->fullKey(key);
    QVariant oldValue = d->settings->value(fullKey);
    if (oldValue != value) {
        d->ignoreNextChange = true;
        d->settings->setValue(fullKey, value);
        d->settings->sync();
        d->cachedValues[fullKey] = value;
        emit valueChanged(fullKey, value);
    }
}

bool UniSettings::contains(const QString &key) const
{
    Q_D(const UniSettings);
    return d->settings->contains(d->fullKey(key));
}

void UniSettings::remove(const QString &key)
{
    Q_D(UniSettings);
    QString fullKey = d->fullKey(key);
    d->ignoreNextChange = true;
    d->settings->remove(fullKey);
    d->settings->sync();
    d->cachedValues.remove(fullKey);
    emit valueChanged(fullKey, QVariant());
}

QStringList UniSettings::allKeys() const
{
    Q_D(const UniSettings);
    return d->settings->allKeys();
}

void UniSettings::clear()
{
    Q_D(UniSettings);
    d->ignoreNextChange = true;
    d->settings->clear();
    d->settings->sync();
    d->cachedValues.clear();
}

void UniSettings::sync()
{
    Q_D(UniSettings);
    d->settings->sync();
}

void UniSettings::beginGroup(const QString &prefix)
{
    Q_D(UniSettings);
    if (!d->currentGroup.isEmpty()) {
        d->currentGroup += "/" + prefix;
    } else {
        d->currentGroup = prefix;
    }
}

void UniSettings::endGroup()
{
    Q_D(UniSettings);
    int idx = d->currentGroup.lastIndexOf('/');
    if (idx >= 0) {
        d->currentGroup = d->currentGroup.left(idx);
    } else {
        d->currentGroup.clear();
    }
}

QString UniSettings::group() const
{
    Q_D(const UniSettings);
    return d->currentGroup;
}

QVariant UniSettings::systemValue(const QString &key, const QVariant &defaultValue) const
{
    Q_D(const UniSettings);
    if (d->scope == SystemScope) {
        return value(key, defaultValue);
    }

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString systemConfigPath = configDir + "/unisettings/system.conf";
    QSettings systemSettings(systemConfigPath, QSettings::IniFormat);
    return systemSettings.value(key, defaultValue);
}

QVariant UniSettings::appValue(const QString &appName, const QString &key, const QVariant &defaultValue) const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString appConfigPath = configDir + "/unisettings/" + appName + ".conf";
    QSettings appSettings(appConfigPath, QSettings::IniFormat);
    return appSettings.value(key, defaultValue);
}

QString UniSettings::applicationName() const
{
    Q_D(const UniSettings);
    return d->appName;
}

UniSettings::Scope UniSettings::scope() const
{
    Q_D(const UniSettings);
    return d->scope;
}

void UniSettings::onFileChanged(const QString &path)
{
    Q_D(UniSettings);
    Q_UNUSED(path);
    if (!d->debounceTimer->isActive()) {
        d->debounceTimer->start();
    }
}
