#include "MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    // ── High-DPI support (Qt 6 enables this by default, but be explicit) ─────
    QApplication app(argc, argv);
    
    // ── Application metadata ─────────────────────────────────────────────────
    QApplication::setApplicationName("AerokosmikGroundStation");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("Mission Kripto");

    // ── Load dark theme stylesheet from Qt resources ──────────────────────────
    QFile qss(":/dark_theme.qss");
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(QLatin1String(qss.readAll()));
        qss.close();
    }

    // ── Prefer Fusion style as a cross-platform dark-theme base ──────────────
    app.setStyle(QStyleFactory::create("Fusion"));

    // ── Main window ───────────────────────────────────────────────────────────
    MainWindow w;
    w.show();

    return app.exec();
}
