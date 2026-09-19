#include "config_manager.h"
#include <QCoreApplication>
#include <QDir>

// Default values matching original Python module.py
static const struct { const char *key; const char *value; } k_defaults[] = {
    { "theme",        "light"    },
    { "font_family",  "Segoe UI" },
    { "font_size",    "16"       },
    { "language",     "en"       },
    { "display_mode", "1"        },
};

ConfigManager::ConfigManager()
    : m_settings(
          QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini"),
          QSettings::IniFormat)
{
    m_settings.setIniCodec("UTF-8");
    loadOrCreate();
}

void ConfigManager::loadOrCreate()
{
    m_settings.beginGroup("General");
    for (const auto &d : k_defaults) {
        if (!m_settings.contains(d.key))
            m_settings.setValue(d.key, d.value);
    }
    m_settings.endGroup();
    m_settings.sync();
}

QString ConfigManager::get(const QString &key, const QString &defaultValue) const
{
    QSettings &s = const_cast<QSettings &>(m_settings);
    return s.value(QStringLiteral("General/") + key, defaultValue).toString();
}

int ConfigManager::getInt(const QString &key, int defaultValue) const
{
    bool ok = false;
    int  v  = get(key).toInt(&ok);
    return ok ? v : defaultValue;
}

void ConfigManager::set(const QString &key, const QString &value)
{
    m_settings.setValue(QStringLiteral("General/") + key, value);
}

void ConfigManager::save()
{
    m_settings.sync();
}
