#include "DeviceWindow.h"
#include "engine/GraphicsEngine.h"
#include "Utility/Qt/QCameraWidget.h"
#include "Utility/Qt/QGocatorWidget.h"
#include "CameraSystem.h"
#include "GraphicsEngineSink.h"
#include "CameraImagingController.h"
#include "GocatorImagingController.h"
#include "QProcessingWidget.h"
#include "StaticImageImagingController.h"
#include "QStaticImageControlWidget.h"
#include <QDockWidget>
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
    // 1. Create GraphicsEngine as central widget
    _graphicsEngine = new GraphicsEngine(this);
    setCentralWidget(_graphicsEngine);

    // 2. Create GraphicsEngineSink
    _sink = new GraphicsEngineSink(_graphicsEngine, this);

    // 3. Create right dock (Control Panel placeholder)
    _controlDock = new QDockWidget(QStringLiteral("Device Control"), this);
    _controlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    auto* dummyControl = new QLabel(QStringLiteral("Control Panel Placeholder"), _controlDock);
    dummyControl->setAlignment(Qt::AlignCenter);
    _controlDock->setWidget(dummyControl);
    addDockWidget(Qt::RightDockWidgetArea, _controlDock);
}

void DeviceWindow::initCamera() {
    if (!_camera) return;

    // Create camera widget and set it to control dock
    _cameraWidget = new QCameraWidget(_controlDock, _camera);
    _controlDock->setWidget(_cameraWidget);

    // Create Camera acquisition controller and bind the sink
    _controller = std::make_unique<CameraImagingController>(_camera, this);
    _controller->setSink(_sink);

    // Create image processing pipeline widget and dock it to the bottom
    _processingWidget = new QProcessingWidget(this);
    _processingWidget->setController(_controller.get());

    _processingDock = new QDockWidget(QStringLiteral("Image Processing Pipeline"), this);
    _processingDock->setWidget(_processingWidget);
    addDockWidget(Qt::BottomDockWidgetArea, _processingDock);
}

void DeviceWindow::initGocator() {
    if (!_gocator) return;

    // Create Gocator widget and set it to control dock
    _gocatorWidget = new QGocatorWidget(_controlDock, _gocator);
    _controlDock->setWidget(_gocatorWidget);

    // Create Gocator acquisition controller and bind the sink
    _controller = std::make_unique<GocatorImagingController>(_gocator, this);
    _controller->setSink(_sink);

    // Create image processing pipeline widget and dock it to the bottom
    _processingWidget = new QProcessingWidget(this);
    _processingWidget->setController(_controller.get());

    _processingDock = new QDockWidget(QStringLiteral("Image Processing Pipeline"), this);
    _processingDock->setWidget(_processingWidget);
    addDockWidget(Qt::BottomDockWidgetArea, _processingDock);
}

void DeviceWindow::initStaticImage(const QStringList& filePaths) {
    // 1. Create static image controller and bind the sink
    auto controller = std::make_unique<StaticImageImagingController>(filePaths, this);
    controller->setSink(_sink);

    // 2. Create static image control widget and host in the control dock
    _staticImageWidget = new QStaticImageControlWidget(_controlDock);
    _staticImageWidget->setController(controller.get());
    _controlDock->setWidget(_staticImageWidget);
    _controlDock->setWindowTitle(QStringLiteral("Test Image Control"));

    // 3. Keep unique_ptr reference in the window
    _controller = std::move(controller);

    // 4. Create image processing pipeline widget and dock to the bottom
    _processingWidget = new QProcessingWidget(this);
    _processingWidget->setController(_controller.get());

    _processingDock = new QDockWidget(QStringLiteral("Image Processing Pipeline"), this);
    _processingDock->setWidget(_processingWidget);
    addDockWidget(Qt::BottomDockWidgetArea, _processingDock);
}
