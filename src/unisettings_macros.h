#ifndef UNISETTINGS_MACROS_H
#define UNISETTINGS_MACROS_H

#include "unisettings.h"

// Base class for creating Uni settings objects
class UniSettingsObject : public QObject {
    Q_OBJECT
protected:
    UniSettings *m_settings;
    
    explicit UniSettingsObject(const QString &appName, QObject *parent = nullptr)
        : QObject(parent)
        , m_settings(new UniSettings(appName, this))
    {
        connect(m_settings, &UniSettings::valueChanged,
                this, &UniSettingsObject::onSettingChanged);
    }
    
    virtual ~UniSettingsObject() = default;
    virtual void onSettingChanged(const QString &key, const QVariant &value) = 0;

public:
    UniSettings* settings() const { return m_settings; }
};

// settings definition beginning macro
#define UNISETTINGS_CLASS(ClassName, AppName) \
class ClassName : public UniSettingsObject { \
    Q_OBJECT \
public: \
    explicit ClassName(QObject *parent = nullptr) \
        : UniSettingsObject(AppName, parent) { \
        loadAllSettings(); \
    } \
private: \
    void loadAllSettings(); \
    void onSettingChanged(const QString &key, const QVariant &value) override;

// macro for defining settings with types
#define UNISETTINGS_PROPERTY(Type, name, key, defaultValue) \
private: \
    Q_PROPERTY(Type name READ name WRITE set##name NOTIFY name##Changed) \
public: \
    Type name() const { \
        return m_settings->value(key, defaultValue).value<Type>(); \
    } \
    void set##name(const Type &value) { \
        if (name() != value) { \
            m_settings->setValue(key, value); \
            emit name##Changed(); \
        } \
    } \
Q_SIGNALS: \
    void name##Changed();

// simplified ver with auto keys
#define UNISETTINGS_PROPERTY_AUTO(Type, name, defaultValue) \
    UNISETTINGS_PROPERTY(Type, name, #name, defaultValue)

// grouping macro
#define UNISETTINGS_PROPERTY_GROUP(Type, name, group, key, defaultValue) \
private: \
    Q_PROPERTY(Type name READ name WRITE set##name NOTIFY name##Changed) \
public: \
    Type name() const { \
        m_settings->beginGroup(group); \
        auto val = m_settings->value(key, defaultValue).value<Type>(); \
        m_settings->endGroup(); \
        return val; \
    } \
    void set##name(const Type &value) { \
        m_settings->beginGroup(group); \
        if (m_settings->value(key, defaultValue).value<Type>() != value) { \
            m_settings->setValue(key, value); \
            m_settings->endGroup(); \
            emit name##Changed(); \
        } else { \
            m_settings->endGroup(); \
        } \
    } \
Q_SIGNALS: \
    void name##Changed();

// ending macro
#define UNISETTINGS_END() \
};

// implementation macros
#define UNISETTINGS_IMPL_BEGIN(ClassName) \
void ClassName::loadAllSettings() {

#define UNISETTINGS_LOAD_PROPERTY(name) \
    if (!m_settings->contains(#name)) { \
        m_settings->setValue(#name, name()); \
    } \
    emit name##Changed();

#define UNISETTINGS_IMPL_CHANGE_BEGIN(ClassName) \
    m_settings->sync(); \
} \
void ClassName::onSettingChanged(const QString &key, const QVariant &value) { \
    Q_UNUSED(value);

#define UNISETTINGS_HANDLE_CHANGE(key, name) \
    if (key == key) emit name##Changed();

#define UNISETTINGS_IMPL_END() \
}

#endif // UNISETTINGS_MACROS_H
