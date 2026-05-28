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
#include "Chrome/DockTitleBar.h"
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QResizeEvent>
#include <QMdiSubWindow>

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
    setCursor(Qt::ArrowCursor);
    setAttribute(Qt::WA_TranslucentBackground);
    _graphicsEngine = new GraphicsEngine(this);
    setCentralWidget(_graphicsEngine);
    _sink = new GraphicsEngineSink(_graphicsEngine, this);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
    _undockedMainWindowWidth = 450;
    _controlDockWasDockedVisible = true;
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
    _controlDock->setTitleBarWidget(new DockTitleBar(_controlDock, _controlDock));
    _controlDock->setWidget(widget);
    addDockWidget(Qt::LeftDockWidgetArea, _controlDock);
    _controlDock->show();

    connect(_controlDock, &QDockWidget::visibilityChanged, this, &DeviceSession::updateControlDockLayoutState);
    connect(_controlDock, &QDockWidget::topLevelChanged, this, &DeviceSession::updateControlDockLayoutState);
}

void DeviceSession::createProcessingDock() {
    _processingWidget = new QProcessingWidget(this);
    _processingWidget->setController(_controller.get());

    _processingDock = new QDockWidget(QStringLiteral("Image Processing Pipeline"), this);
    _processingDock->setObjectName(QStringLiteral("ProcessingPipelineDock"));
    _processingDock->setTitleBarWidget(new DockTitleBar(_processingDock, _processingDock));
    _processingDock->setWidget(_processingWidget);
    addDockWidget(Qt::RightDockWidgetArea, _processingDock);

    _processingDock->hide(); // Default HIDE

    connect(_processingDock, &QDockWidget::visibilityChanged, this, &DeviceSession::updateProcessingDockLayoutState);
    connect(_processingDock, &QDockWidget::topLevelChanged, this, &DeviceSession::updateProcessingDockLayoutState);
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

void DeviceSession::updateControlDockLayoutState() {
    if (!_controlDock) return;

    updateGeometry();

    bool isDockedVisible = _controlDock->isVisible() && !_controlDock->isFloating();
    if (isDockedVisible == _controlDockWasDockedVisible) return;

    _controlDockWasDockedVisible = isDockedVisible;

    QTimer::singleShot(50, this, [this, isDockedVisible]() {
        if (!_controlDock) return;
        bool currentDockedVisible = _controlDock->isVisible() && !_controlDock->isFloating();
        if (currentDockedVisible != isDockedVisible) return;

        updateGeometry();
        adjustSessionWidth();
    });
}

void DeviceSession::updateProcessingDockLayoutState() {
    if (!_processingDock) return;

    updateGeometry();

    bool isDockedVisible = _processingDock->isVisible() && !_processingDock->isFloating();
    if (isDockedVisible == _processingDockWasDockedVisible) return;

    _processingDockWasDockedVisible = isDockedVisible;

    QTimer::singleShot(50, this, [this, isDockedVisible]() {
        if (!_processingDock) return;
        bool currentDockedVisible = _processingDock->isVisible() && !_processingDock->isFloating();
        if (currentDockedVisible != isDockedVisible) return;

        updateGeometry();
        adjustSessionWidth();
    });
}

void DeviceSession::adjustSessionWidth() {
    bool controlDockedVisible = _controlDock && _controlDock->isVisible() && !_controlDock->isFloating();
    bool processingDockedVisible = _processingDock && _processingDock->isVisible() && !_processingDock->isFloating();

    int controlWidth = _controlDock && _controlDock->widget() ? _controlDock->widget()->sizeHint().width() : 300;
    if (controlWidth <= 0) controlWidth = 300;
    controlWidth = qBound(300, controlWidth, 340);
    controlWidth += 12;

    int processingWidth = _processingDock && _processingDock->widget() ? _processingDock->widget()->sizeHint().width() : 320;
    if (processingWidth <= 0) processingWidth = 320;
    processingWidth = qBound(320, processingWidth, 360);
    processingWidth += 12;

    int targetW = _undockedMainWindowWidth;
    if (controlDockedVisible) targetW += controlWidth;
    if (processingDockedVisible) targetW += processingWidth;

    bool isMax = _subWindow ? _subWindow->isMaximized() : isMaximized();
    if (!isMax && _subWindow) {
        int targetH = _subWindow->height();

        if (processingDockedVisible && _processingDock->widget()) {
            int minH = _processingDock->widget()->minimumSizeHint().height() + 45;
            targetH = qMax(targetH, minH);
        }
        if (controlDockedVisible && _controlDock->widget()) {
            int minH = _controlDock->widget()->minimumSizeHint().height() + 45;
            targetH = qMax(targetH, minH);
        }

        _subWindow->resize(targetW, targetH);

        if (controlDockedVisible) {
            resizeDocks({_controlDock}, {controlWidth}, Qt::Horizontal);
        }
        if (processingDockedVisible) {
            resizeDocks({_processingDock}, {processingWidth}, Qt::Horizontal);
        }
    }
}

void DeviceSession::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
}

QSize DeviceSession::minimumSizeHint() const {
    int minW = 300;
    int minH = 200;

    if (_graphicsEngine) {
        minW = qMax(minW, _graphicsEngine->minimumSize().width() > 0 ? _graphicsEngine->minimumSize().width() : _graphicsEngine->minimumSizeHint().width());
        minH = qMax(minH, _graphicsEngine->minimumSize().height() > 0 ? _graphicsEngine->minimumSize().height() : _graphicsEngine->minimumSizeHint().height());
    }

    if (_controlDock && _controlDock->isVisible() && !_controlDock->isFloating()) {
        QWidget* w = _controlDock->widget();
        if (w) {
            int wW = w->minimumSize().width() > 0 ? w->minimumSize().width() : w->minimumSizeHint().width();
            int wH = w->minimumSize().height() > 0 ? w->minimumSize().height() : w->minimumSizeHint().height();
            minW += wW;
            minH = qMax(minH, wH);
        }
    }

    if (_processingDock && _processingDock->isVisible() && !_processingDock->isFloating()) {
        QWidget* w = _processingDock->widget();
        if (w) {
            int wW = w->minimumSize().width() > 0 ? w->minimumSize().width() : w->minimumSizeHint().width();
            int wH = w->minimumSize().height() > 0 ? w->minimumSize().height() : w->minimumSizeHint().height();
            minW += wW;
            minH = qMax(minH, wH);
        }
    }

    // Add standard margins for splitters/borders/menus
    minW += 16;
    minH += 38;

    return QSize(minW, minH);
}

void DeviceSession::notifyManualResizeFinished() {
    bool isMax = _subWindow ? _subWindow->isMaximized() : isMaximized();
    if (!isMax && _subWindow && !_subWindow->isMinimized()) {
        bool controlDockedVisible = _controlDock && _controlDock->isVisible() && !_controlDock->isFloating();
        bool processingDockedVisible = _processingDock && _processingDock->isVisible() && !_processingDock->isFloating();

        int currentW = _subWindow->width();

        int controlWidth = 0;
        if (controlDockedVisible) {
            controlWidth = _controlDock->widget() ? _controlDock->widget()->sizeHint().width() : 300;
            if (controlWidth <= 0) controlWidth = 300;
            controlWidth += 12;
        }

        int processingWidth = 0;
        if (processingDockedVisible) {
            processingWidth = _processingDock->widget() ? _processingDock->widget()->sizeHint().width() : 320;
            if (processingWidth <= 0) processingWidth = 320;
            processingWidth += 12;
        }

        int computedUndockedW = currentW - controlWidth - processingWidth;
        if (computedUndockedW > 250) {
            _undockedMainWindowWidth = computedUndockedW;
        }
    }
}
