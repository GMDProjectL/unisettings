#ifndef UNISETTINGS_H
#define UNISETTINGS_H

#include "unisettings_global.h"
#include <QObject>
#include <QString>
#include <QVariant>
#include <QStringList>
#include <memory>

class UniSettingsPrivate;

class UNISETTINGS_EXPORT UniSettings : public QObject
{
    Q_OBJECT

public:
    enum Scope {
        SystemScope,        // DE settings
        ApplicationScope    // app settings
    };

    static UniSettings* instance();
    explicit UniSettings(const QString &appName, QObject *parent = nullptr);
    ~UniSettings();

    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    bool contains(const QString &key) const;
    void remove(const QString &key);
    QStringList allKeys() const;
    void clear();
    void sync();

    void beginGroup(const QString &prefix);
    void endGroup();
    QString group() const;

    // get settings from other scope
    QVariant systemValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    QVariant appValue(const QString &appName, const QString &key, const QVariant &defaultValue = QVariant()) const;

    QString applicationName() const;
    Scope scope() const;

signals:
    void valueChanged(const QString &key, const QVariant &value);
    void externalValueChanged(const QString &appName, const QString &key, const QVariant &value);

private:
    explicit UniSettings(QObject *parent = nullptr); // singleton constr
    UniSettings(const UniSettings&) = delete;
    UniSettings& operator=(const UniSettings&) = delete;

    std::unique_ptr<UniSettingsPrivate> d_ptr;
    Q_DECLARE_PRIVATE(UniSettings)

private slots:
    void onFileChanged(const QString &path);
};

#endif // UNISETTINGS_H