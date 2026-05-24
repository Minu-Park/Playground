#include "DeviceSession.h"
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
#include <QMenu>
#include <QAction>

DeviceSession::DeviceSession(Camera* camera, CameraSystem* cameraSystem, QWidget* parent)
    : QMainWindow(parent), _camera(camera), _cameraSystem(cameraSystem), _gocator(nullptr) {
    initCommon();
    initCamera();
}

DeviceSession::DeviceSession(Gocator* gocator, QWidget* parent)
    : QMainWindow(parent), _camera(nullptr), _cameraSystem(nullptr), _gocator(gocator) {
    initCommon();
    initGocator();
}

DeviceSession::DeviceSession(const QStringList& filePaths, QWidget* parent)
    : QMainWindow(parent), _camera(nullptr), _cameraSystem(nullptr), _gocator(nullptr) {
    initCommon();
    initStaticImage(filePaths);
}

DeviceSession::~DeviceSession() {
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

void DeviceSession::initCommon() {
    _graphicsEngine = new GraphicsEngine(this);
    setCentralWidget(_graphicsEngine);
    _sink = new GraphicsEngineSink(_graphicsEngine, this);
}

void DeviceSession::initCamera() {
    if (!_camera) return;

    _cameraWidget = new QCameraWidget(this, _camera);
    setControlWidget(_cameraWidget, QStringLiteral("Camera Controls"));

    _controller = std::make_unique<CameraImagingController>(_camera, this);
    _controller->setSink(_sink);

    createProcessingDock();
    setupViewMenu();
}

void DeviceSession::initGocator() {
    if (!_gocator) return;

    _gocatorWidget = new QGocatorWidget(this, _gocator);
    setControlWidget(_gocatorWidget, QStringLiteral("Gocator Controls"));

    _controller = std::make_unique<GocatorImagingController>(_gocator, this);
    _controller->setSink(_sink);

    createProcessingDock();
    setupViewMenu();
}

void DeviceSession::initStaticImage(const QStringList& filePaths) {
    auto controller = std::make_unique<StaticImageImagingController>(filePaths, this);
    controller->setSink(_sink);

    _staticImageWidget = new QStaticImageControlWidget(this);
    _staticImageWidget->setController(controller.get());
    setControlWidget(_staticImageWidget, QStringLiteral("Test Image Controls"));

    _controller = std::move(controller);

    createProcessingDock();
    setupViewMenu();
}

void DeviceSession::setControlWidget(QWidget* widget, const QString& title) {
    _controlDock = new QDockWidget(title, this);
    _controlDock->setObjectName(QStringLiteral("SessionControlDock"));
    _controlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    _controlDock->setWidget(widget);
    addDockWidget(Qt::LeftDockWidgetArea, _controlDock);
    _controlDock->show();
}

void DeviceSession::createProcessingDock() {
    _processingWidget = new QProcessingWidget(this);
    _processingWidget->setController(_controller.get());

    _processingDock = new QDockWidget(QStringLiteral("Image Processing Pipeline"), this);
    _processingDock->setObjectName(QStringLiteral("ProcessingPipelineDock"));
    _processingDock->setWidget(_processingWidget);
    addDockWidget(Qt::BottomDockWidgetArea, _processingDock);
    _processingDock->hide(); // Default HIDE
}

void DeviceSession::setupViewMenu() {
    QMenuBar* deviceMenuBar = menuBar();
    deviceMenuBar->setNativeMenuBar(false);
    QMenu* viewMenu = deviceMenuBar->addMenu(QStringLiteral("&View"));

    if (_controlDock) {
        QAction* toggleControlAct = _controlDock->toggleViewAction();
        toggleControlAct->setText(QStringLiteral("Control Panel"));
        viewMenu->addAction(toggleControlAct);
    }

    if (_processingDock) {
        QAction* toggleProcAct = _processingDock->toggleViewAction();
        toggleProcAct->setText(QStringLiteral("Image Processing Pipeline"));
        viewMenu->addAction(toggleProcAct);
    }
}
