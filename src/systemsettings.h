#ifndef SYSTEMSETTINGS_H
#define SYSTEMSETTINGS_H

#include <QObject>
#include <QVariant>
#include "unisettings.h"

class SystemSettings : public QObject
{
    Q_OBJECT
    
public:
    static SystemSettings* instance();
    
    // QML-accessible methods
    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE bool contains(const QString &key) const;
    Q_INVOKABLE void remove(const QString &key);
    Q_INVOKABLE QStringList allKeys() const;
    
    // For reading other app settings from system scope
    Q_INVOKABLE QVariant appValue(const QString &appName, const QString &key, 
                                   const QVariant &defaultValue = QVariant()) const;

signals:
    // Generic signal for any system setting change
    void settingChanged(const QString &key, const QVariant &value);
    
    // App-specific change signal
    void appSettingChanged(const QString &appName, const QString &key, const QVariant &value);
    
private:
    explicit SystemSettings(QObject *parent = nullptr);
    SystemSettings(const SystemSettings&) = delete;
    SystemSettings& operator=(const SystemSettings&) = delete;
    
    UniSettings *m_settings;
};

#endif // SYSTEMSETTINGS_H
