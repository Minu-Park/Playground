#include "DeviceWindow.h"
#include "engine/GraphicsEngine.h"
#include "Utility/Qt/QCameraWidget.h"
#include "Utility/Qt/QGocatorWidget.h"
#include "CameraSystem.h"
#include "Controller/GraphicsEngineSink.h"
#include "Controller/CameraImagingController.h"
#include "Controller/GocatorImagingController.h"
#include "UI/QProcessingWidget.h"
#include "Controller/StaticImageImagingController.h"
#include "UI/QStaticImageControlWidget.h"
#include <QDockWidget>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QLabel>
#include <QPointer>
#include <QDebug>

DeviceWindow::DeviceWindow(Camera* camera, CameraSystem* cameraSystem, QWidget* parent)
    : QMainWindow(parent), _camera(camera), _cameraSystem(cameraSystem), _gocator(nullptr) {
    initCommon();
    initCamera();
}

DeviceWindow::DeviceWindow(Gocator* gocator, QWidget* parent)
    : QMainWindow(parent), _camera(nullptr), _cameraSystem(nullptr), _gocator(gocator) {
    initCommon();
    initGocator();
}

DeviceWindow::DeviceWindow(const QStringList& filePaths, QWidget* parent)
    : QMainWindow(parent), _camera(nullptr), _cameraSystem(nullptr), _gocator(nullptr) {
    initCommon();
    initStaticImage(filePaths);
}

DeviceWindow::~DeviceWindow() {
    if (_camera && _cameraWidget) {
        _cameraWidget->prepareForShutdown();
    }
    if (_gocator && _gocatorWidget) {
        _gocatorWidget->prepareForShutdown();
    }

    // Destroy the controller first to safely stop grabbing and deregister callbacks
    _controller.reset();

    // Now safely delete/remove the devices
    if (_camera) {
        if (_cameraSystem) {
            _cameraSystem->removeCamera(_camera);
        } else {
            delete _camera;
        }
        _camera = nullptr;
    }

    if (_gocator) {
        _gocator->close();
        delete _gocator;
        _gocator = nullptr;
    }
}

void DeviceWindow::initCommon() {
    // 1. Create GraphicsEngine
    _graphicsEngine = new GraphicsEngine(this);

    // 2. Wrap GraphicsEngine in a Dock Widget
    _graphicsEngineDock = new QDockWidget(QStringLiteral("Live Viewer"), this);
    _graphicsEngineDock->setObjectName(QStringLiteral("LiveViewerDock"));
    _graphicsEngineDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    _graphicsEngineDock->setWidget(_graphicsEngine);
    addDockWidget(Qt::LeftDockWidgetArea, _graphicsEngineDock);
    _graphicsEngineDock->show(); // Default ON

    // 3. Create GraphicsEngineSink
    _sink = new GraphicsEngineSink(_graphicsEngine, this);
}

void DeviceWindow::initCamera() {
    if (!_camera) return;

    // Create camera widget as the central widget
    _cameraWidget = new QCameraWidget(this, _camera);
    setCentralWidget(_cameraWidget);

    // Create Camera acquisition controller and bind the sink
    _controller = std::make_unique<CameraImagingController>(_camera, this);
    _controller->setSink(_sink);

    // Create image processing pipeline widget and dock it to the bottom
    _processingWidget = new QProcessingWidget(this);
    _processingWidget->setController(_controller.get());

    _processingDock = new QDockWidget(QStringLiteral("Image Processing Pipeline"), this);
    _processingDock->setObjectName(QStringLiteral("ProcessingPipelineDock"));
    _processingDock->setWidget(_processingWidget);
    addDockWidget(Qt::BottomDockWidgetArea, _processingDock);
    _processingDock->hide(); // Default HIDE

    setupViewMenus();
}

void DeviceWindow::initGocator() {
    if (!_gocator) return;

    // Create Gocator widget as the central widget
    _gocatorWidget = new QGocatorWidget(this, _gocator);
    setCentralWidget(_gocatorWidget);

    // Create Gocator acquisition controller and bind the sink
    _controller = std::make_unique<GocatorImagingController>(_gocator, this);
    _controller->setSink(_sink);

    // Create image processing pipeline widget and dock it to the bottom
    _processingWidget = new QProcessingWidget(this);
    _processingWidget->setController(_controller.get());

    _processingDock = new QDockWidget(QStringLiteral("Image Processing Pipeline"), this);
    _processingDock->setObjectName(QStringLiteral("ProcessingPipelineDock"));
    _processingDock->setWidget(_processingWidget);
    addDockWidget(Qt::BottomDockWidgetArea, _processingDock);
    _processingDock->hide(); // Default HIDE

    setupViewMenus();
}

void DeviceWindow::initStaticImage(const QStringList& filePaths) {
    // 1. Create static image controller and bind the sink
    auto controller = std::make_unique<StaticImageImagingController>(filePaths, this);
    controller->setSink(_sink);

    // 2. Create static image control widget as the central widget
    _staticImageWidget = new QStaticImageControlWidget(this);
    _staticImageWidget->setController(controller.get());
    setCentralWidget(_staticImageWidget);

    // 3. Keep unique_ptr reference in the window
    _controller = std::move(controller);

    // 4. Create image processing pipeline widget and dock to the bottom
    _processingWidget = new QProcessingWidget(this);
    _processingWidget->setController(_controller.get());

    _processingDock = new QDockWidget(QStringLiteral("Image Processing Pipeline"), this);
    _processingDock->setObjectName(QStringLiteral("ProcessingPipelineDock"));
    _processingDock->setWidget(_processingWidget);
    addDockWidget(Qt::BottomDockWidgetArea, _processingDock);
    _processingDock->hide(); // Default HIDE

    setupViewMenus();
}

void DeviceWindow::setupViewMenus() {
    // Create menu bar inside DeviceWindow if not already present
    auto* menu = menuBar();
    auto* viewMenu = menu->addMenu(QStringLiteral("&View"));

    QAction* toggleGeAct = _graphicsEngineDock->toggleViewAction();
    toggleGeAct->setText(QStringLiteral("Show Live Viewer"));
    toggleGeAct->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-view-48.png")));
    viewMenu->addAction(toggleGeAct);

    if (_processingDock) {
        QAction* toggleProcAct = _processingDock->toggleViewAction();
        toggleProcAct->setText(QStringLiteral("Show Image Processing"));
        toggleProcAct->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-photo-editor-48.png")));
        viewMenu->addAction(toggleProcAct);
    }

    // Add a toolbar for quick toggle access
    auto* viewToolBar = addToolBar(QStringLiteral("View Controls"));
    viewToolBar->setObjectName(QStringLiteral("DeviceWindowViewToolBar"));
    viewToolBar->setIconSize(QSize(20, 20));
    viewToolBar->setMovable(false);

    viewToolBar->addAction(toggleGeAct);
    if (_processingDock) {
        viewToolBar->addAction(_processingDock->toggleViewAction());
    }
}
