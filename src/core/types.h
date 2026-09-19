#pragma once
#include <QString>

namespace FeReader {

enum class BookType {
    None,
    Pdf,
    Epub
};

enum class ViewOrientation {
    Vertical,
    Horizontal
};

static const QString APP_VERSION = QStringLiteral("3.1.2");
static const QString ORG_NAME    = QStringLiteral("Neofilisoft");
static const QString APP_NAME    = QStringLiteral("FeReader");

} // namespace FeReader
