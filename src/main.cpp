#include <QApplication>
#include <QIcon>
#include <QSettings>
#include "ui/main_window.h"
#include "core/types.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(FeReader::ORG_NAME);
    app.setApplicationName(FeReader::APP_NAME);
    app.setApplicationVersion(FeReader::APP_VERSION);
    app.setWindowIcon(QIcon(QStringLiteral(":/icon.ico")));

    MainWindow window;

    // Restore display mode from config (matches Python main())
    QString mode = window.configManager().get("display_mode", "1");
    if (mode == "2")      window.showFullScreen();
    else if (mode == "1") window.showMaximized();
    else                  window.show();

    if (argc > 1) {
        window.loadFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
