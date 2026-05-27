#include "MainWindow.h"
#include "Resources.h"
#include "Utility/LogManager.h"
#include "engine/GraphicsEngine.h"
#include <QApplication>
#include <QFont>

int main(int argc, char* argv[])
{
    // Configure the VTK/GL default before a visible host window can be refreshed
    // by the first session-created OpenGL widget.
    GraphicsEngine::installDefaultSurfaceFormat();

    QApplication app(argc, argv);

    // Initialize global log manager after QApplication is created
    LogManager::instance()->init();

    Resources::installResources(app);

    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
