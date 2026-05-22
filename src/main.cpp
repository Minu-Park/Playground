#include "MainWindow.h"
#include "Resources.h"
#include "LogManager.h"
#include <QApplication>
#include <QFont>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

#if defined(__APPLE__)
    // Explicitly set default font to avoid "Sans Serif" alias lookup penalty (51ms) on macOS
    QFont defaultFont(QStringLiteral(".AppleSystemUIFont"), 12);
    QApplication::setFont(defaultFont);
#endif

    // Initialize global log manager after QApplication is created
    LogManager::instance()->init();

    Resources::installResources(app);

    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
