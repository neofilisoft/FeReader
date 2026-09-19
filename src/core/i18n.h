#pragma once
#include <QString>
#include <QMap>

namespace I18n {

// Return localized string for key in given lang ("en" or "th")
QString tr(const QString &key, const QString &lang);

// Returns all keys for the given lang
QMap<QString, QString> stringsFor(const QString &lang);

} // namespace I18n
