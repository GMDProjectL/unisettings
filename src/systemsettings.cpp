#include "systemsettings.h"

static SystemSettings *s_systemInstance = nullptr;

SystemSettings* SystemSettings::instance()
{
    if (!s_systemInstance) {
        s_systemInstance = new SystemSettings(nullptr);
    }
    return s_systemInstance;
}

SystemSettings::SystemSettings(QObject *parent)
    : QObject(parent)
    , m_settings(UniSettings::instance())  // Use the singleton
{
    connect(m_settings, &UniSettings::valueChanged,
            this, [this](const QString &key, const QVariant &value) {
        emit settingChanged(key, value);
    });
    
    // Connect to system-wide changes
    connect(m_settings, &UniSettings::externalValueChanged,
            this, [this](const QString &appName, const QString &key, const QVariant &value) {
        if (appName == "system") {
            emit settingChanged(key, value);
        } else {
            emit appSettingChanged(appName, key, value);
        }
    });
}

QVariant SystemSettings::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void SystemSettings::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
}

bool SystemSettings::contains(const QString &key) const
{
    return m_settings->contains(key);
}

void SystemSettings::remove(const QString &key)
{
    m_settings->remove(key);
}

QStringList SystemSettings::allKeys() const
{
    return m_settings->allKeys();
}

QVariant SystemSettings::appValue(const QString &appName, const QString &key, 
                                   const QVariant &defaultValue) const
{
    return m_settings->appValue(appName, key, defaultValue);
}
