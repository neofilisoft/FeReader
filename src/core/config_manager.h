#pragma once
#include <QString>
#include <QSettings>

// Manages persistent settings stored in settings.ini next to the executable.
// Mirrors the Python ConfigManager class exactly.
class ConfigManager
{
public:
    ConfigManager();

    QString get(const QString &key, const QString &defaultValue = QString()) const;
    void    set(const QString &key, const QString &value);
    void    save();

    // Convenience typed getters
    int     getInt(const QString &key, int defaultValue = 0) const;

private:
    void loadOrCreate();

    QString   m_path;          // Full path to settings.ini
    QSettings m_settings;
};
