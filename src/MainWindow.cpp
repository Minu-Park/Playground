#include "MainWindow.h"
#include "DeviceSession.h"
#include "CameraSystem.h"
#include "Gocator.h"
#include "Chrome/DockTitleBar.h"
#include "Chrome/MainTitleBar.h"
#include "Chrome/MdiSubWindowContainer.h"
#include "Utility/LogManager.h"
#include <QAction>
#include <QWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QLayout>
#include <QDockWidget>
#include <QStatusBar>
#include <QTextEdit>
#include <QScrollBar>
#include <QLabel>
#include <QSettings>
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QMouseEvent>
#include <algorithm>

namespace {

class BrandedMdiArea final : public QMdiArea {
public:
    explicit BrandedMdiArea(QWidget* parent = nullptr)
        : QMdiArea(parent)
        , _logo(QStringLiteral(":/Resources/BASLER_Logo.png"))
    {
        setCursor(Qt::ArrowCursor);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.fillRect(viewport()->rect(), QColor(QStringLiteral("#eeeeee")));

        if (_logo.isNull()) return;

        const QSize logoSize = _logo.size() / 2;
        const QPoint topLeft(
            (viewport()->width() - logoSize.width()) / 2,
            (viewport()->height() - logoSize.height()) / 2);
        painter.drawPixmap(QRect(topLeft, logoSize), _logo);
    }

private:
    QPixmap _logo;
};

class OpenGLCompositionSeed final : public QOpenGLWidget, protected QOpenGLFunctions {
public:
    explicit OpenGLCompositionSeed(QWidget* parent)
        : QOpenGLWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFocusPolicy(Qt::NoFocus);
        setFixedSize(1, 1);
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
    }

    void paintGL() override {
        glClearColor(0.0F, 0.0F, 0.0F, 0.0F);
        glClear(GL_COLOR_BUFFER_BIT);
    }
};

void installRecursiveEventFilter(QWidget* widget, QObject* filter) {
    if (!widget) return;
    widget->setMouseTracking(true);
    widget->installEventFilter(filter);
    const auto children = widget->findChildren<QWidget*>();
    for (QWidget* child : children) {
        child->setMouseTracking(true);
        child->installEventFilter(filter);
    }
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);

    setWindowTitle(QStringLiteral("Basler Playground"));
    resize(1600, 900);

    _cameraSystem = std::make_unique<CameraSystem>();

    _mdiArea = new BrandedMdiArea(this);
    _mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(_mdiArea);
    createOpenGLCompositionSeed();
    installRecursiveEventFilter(_mdiArea, this);

    auto* mainStatusBar = statusBar();
    mainStatusBar->setObjectName(QStringLiteral("MainStatusBar"));
    mainStatusBar->setSizeGripEnabled(false);
    installRecursiveEventFilter(mainStatusBar, this);
    createMainStatusBar();
    connect(_mdiArea, &QMdiArea::subWindowActivated, this, [this]() {
        updateMainStatusBar();
    });

    createLogDock();
    createMenus();

    menuBar()->setNativeMenuBar(false);
    auto* titleBar = new MainTitleBar(this, menuBar(), this);
    installRecursiveEventFilter(titleBar, this);
    setMenuWidget(titleBar);

    connect(LogManager::instance(), &LogManager::logAdded, this, &MainWindow::appendLog, Qt::QueuedConnection);
}

void MainWindow::createOpenGLCompositionSeed() {
    // Qt can recreate a visible top-level window when its first QOpenGLWidget
    // arrives later. Seed compositing before MainWindow is first shown.
    auto* seed = new OpenGLCompositionSeed(_mdiArea->viewport());
    seed->setObjectName(QStringLiteral("OpenGLCompositionSeed"));
    seed->move(0, 0);
    seed->show();
}

MainWindow::~MainWindow() {
    if (_mdiArea) {
        const QList<QMdiSubWindow*> subWindows = _mdiArea->subWindowList();
        for (QMdiSubWindow* subWindow : subWindows) {
            delete subWindow;
        }
    }
}

void MainWindow::createMenus() {
    QMenu* deviceMenu = menuBar()->addMenu(tr("&Device"));

    auto* addCameraAct = new QAction(tr("Add Basler Camera"), this);
    connect(addCameraAct, &QAction::triggered, this, &MainWindow::onAddBaslerCamera);
    deviceMenu->addAction(addCameraAct);

    auto* addGocatorAct = new QAction(tr("Add LMI Gocator"), this);
    connect(addGocatorAct, &QAction::triggered, this, &MainWindow::onAddLmiGocator);
    deviceMenu->addAction(addGocatorAct);

    deviceMenu->addSeparator();

    auto* addTestImagesAct = new QAction(tr("Add Test Image Session..."), this);
    connect(addTestImagesAct, &QAction::triggered, this, &MainWindow::onAddTestImageSession);
    deviceMenu->addAction(addTestImagesAct);

    QMenu* windowMenu = menuBar()->addMenu(tr("&Window"));
    auto* tileAct = new QAction(tr("Tile Windows"), this);
    connect(tileAct, &QAction::triggered, this, &MainWindow::onTileWindows);
    windowMenu->addAction(tileAct);

    auto* cascadeAct = new QAction(tr("Cascade Windows"), this);
    connect(cascadeAct, &QAction::triggered, _mdiArea, &QMdiArea::cascadeSubWindows);
    windowMenu->addAction(cascadeAct);

    if (_logDock) {
        windowMenu->addSeparator();
        QAction* toggleLogAct = _logDock->toggleViewAction();
        toggleLogAct->setText(tr("System Logs"));
        windowMenu->addAction(toggleLogAct);
    }
}



void MainWindow::onAddBaslerCamera() {
    Camera* camera = _cameraSystem->addCamera();
    auto* session = new DeviceSession(camera, _cameraSystem.get(), nullptr);
    session->setAttribute(Qt::WA_DeleteOnClose);
    session->setWindowTitle(QStringLiteral("Basler Camera Session"));
    session->setObjectName(QStringLiteral("DeviceSessionWindow"));

    auto* subWin = new QMdiSubWindow(_mdiArea);
    subWin->setAttribute(Qt::WA_DeleteOnClose);
    subWin->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    subWin->setContentsMargins(0, 0, 0, 0);
    subWin->setWindowTitle(session->windowTitle());
    subWin->setProperty("sessionType", QStringLiteral("Camera"));
    connect(subWin, &QObject::destroyed, this, [this]() {
        QTimer::singleShot(0, this, [this]() { updateMainStatusBar(); });
    });

    session->setSubWindow(subWin);

    auto* container = new MdiSubWindowContainer(subWin, session, session->menuBar(), subWin);
    subWin->setWidget(container);

    if (subWin->layout()) {
        subWin->layout()->setContentsMargins(0, 0, 0, 0);
        subWin->layout()->setSpacing(0);
    }

    _mdiArea->addSubWindow(subWin);
    subWin->show();
    updateMainStatusBar();
}

void MainWindow::onAddLmiGocator() {
    auto* gocator = new Gocator();
    auto* session = new DeviceSession(gocator, nullptr);
    session->setAttribute(Qt::WA_DeleteOnClose);
    session->setWindowTitle(QStringLiteral("LMI Gocator Session"));
    session->setObjectName(QStringLiteral("DeviceSessionWindow"));

    auto* subWin = new QMdiSubWindow(_mdiArea);
    subWin->setAttribute(Qt::WA_DeleteOnClose);
    subWin->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    subWin->setContentsMargins(0, 0, 0, 0);
    subWin->setWindowTitle(session->windowTitle());
    subWin->setProperty("sessionType", QStringLiteral("Gocator"));
    connect(subWin, &QObject::destroyed, this, [this]() {
        QTimer::singleShot(0, this, [this]() { updateMainStatusBar(); });
    });

    session->setSubWindow(subWin);

    auto* container = new MdiSubWindowContainer(subWin, session, session->menuBar(), subWin);
    subWin->setWidget(container);

    if (subWin->layout()) {
        subWin->layout()->setContentsMargins(0, 0, 0, 0);
        subWin->layout()->setSpacing(0);
    }

    _mdiArea->addSubWindow(subWin);
    subWin->show();
    updateMainStatusBar();
}

void MainWindow::onAddTestImageSession() {
    auto* session = new DeviceSession(QStringList(), nullptr);
    session->setAttribute(Qt::WA_DeleteOnClose);
    session->setWindowTitle(QStringLiteral("Test Images Session"));
    session->setObjectName(QStringLiteral("DeviceSessionWindow"));

    auto* subWin = new QMdiSubWindow(_mdiArea);
    subWin->setAttribute(Qt::WA_DeleteOnClose);
    subWin->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    subWin->setContentsMargins(0, 0, 0, 0);
    subWin->setWindowTitle(session->windowTitle());
    subWin->setProperty("sessionType", QStringLiteral("Images"));
    connect(subWin, &QObject::destroyed, this, [this]() {
        QTimer::singleShot(0, this, [this]() { updateMainStatusBar(); });
    });

    session->setSubWindow(subWin);

    auto* container = new MdiSubWindowContainer(subWin, session, session->menuBar(), subWin);
    subWin->setWidget(container);

    if (subWin->layout()) {
        subWin->layout()->setContentsMargins(0, 0, 0, 0);
        subWin->layout()->setSpacing(0);
    }

    _mdiArea->addSubWindow(subWin);
    subWin->show();
    updateMainStatusBar();
}

void MainWindow::createMainStatusBar() {
    _sessionCountStatus = new QLabel(this);
    _sessionCountStatus->setObjectName(QStringLiteral("MainSessionCountStatus"));

    _activeSessionStatus = new QLabel(this);
    _activeSessionStatus->setObjectName(QStringLiteral("MainActiveSessionStatus"));

    statusBar()->addWidget(_sessionCountStatus);
    statusBar()->addWidget(_activeSessionStatus);
    refreshPluginIndicators();
    updateMainStatusBar();
}

void MainWindow::updateMainStatusBar() {
    const int sessionCount = _mdiArea ? _mdiArea->subWindowList().size() : 0;
    if (_sessionCountStatus) {
        _sessionCountStatus->setText(sessionCount == 1 ? tr("1 Session") : tr("%1 Sessions").arg(sessionCount));
    }

    QString activeText = tr("None");
    if (_mdiArea) {
        if (auto* active = _mdiArea->activeSubWindow()) {
            const QString sessionType = active->property("sessionType").toString();
            activeText = sessionType.isEmpty() ? tr("Session") : sessionType;
        }
    }
    if (_activeSessionStatus) {
        _activeSessionStatus->setText(activeText);
    }
}

void MainWindow::refreshPluginIndicators() {
    for (QLabel* label : _pluginIndicators) {
        statusBar()->removeWidget(label);
        delete label;
    }
    _pluginIndicators.clear();

    struct PluginProbe {
        QString name;
        QString objectName;
        bool available;
    };

    QSettings settings;
    const bool opencvAvailable =
#ifdef OPENCV_INCLUDE_DIR
        true
#else
        settings.contains(QStringLiteral("RuntimePaths/OpenCV/IncludeDir"))
        || settings.contains(QStringLiteral("RuntimePaths/OpenCV/LibraryDir"))
#endif
        ;

    QVector<PluginProbe> probes = {
        { QStringLiteral("OpenCV"), QStringLiteral("PluginIndicator_OpenCV"), opencvAvailable },
    };

    for (const auto& probe : probes) {
        if (!probe.available) continue;
        auto* label = new QLabel(probe.name, this);
        label->setObjectName(probe.objectName);
        statusBar()->addPermanentWidget(label);
        _pluginIndicators.append(label);
    }
}

void MainWindow::onTileWindows() {
    QList<QMdiSubWindow*> windows;
    const QList<QMdiSubWindow*> subWindows = _mdiArea->subWindowList();
    for (QMdiSubWindow* subWindow : subWindows) {
        if (!subWindow || !subWindow->isVisible() || subWindow->isMinimized()) {
            continue;
        }
        windows.append(subWindow);
    }

    if (windows.isEmpty()) {
        return;
    }

    std::sort(windows.begin(), windows.end(), [](const QMdiSubWindow* lhs, const QMdiSubWindow* rhs) {
        const QPoint lhsCenter = lhs->geometry().center();
        const QPoint rhsCenter = rhs->geometry().center();
        if (lhsCenter.y() == rhsCenter.y()) {
            return lhsCenter.x() < rhsCenter.x();
        }
        return lhsCenter.y() < rhsCenter.y();
    });

    int columns = 1;
    while (columns * columns < windows.size()) {
        ++columns;
    }
    const int rows = (windows.size() + columns - 1) / columns;
    const QRect area = _mdiArea->viewport()->rect();
    const int cellWidth = area.width() / columns;
    const int cellHeight = area.height() / rows;

    for (int index = 0; index < windows.size(); ++index) {
        QMdiSubWindow* window = windows.at(index);
        if (window->isMaximized()) {
            window->showNormal();
        }

        const int row = index / columns;
        const int column = index % columns;
        const int x = column * cellWidth;
        const int y = row * cellHeight;
        const int width = (column == columns - 1) ? area.width() - x : cellWidth;
        const int height = (row == rows - 1) ? area.height() - y : cellHeight;
        window->setGeometry(x, y, width, height);
    }
}

void MainWindow::createLogDock() {
    _logDock = new QDockWidget(tr("System Logs"), this);
    _logDock->setObjectName(QStringLiteral("SystemLogsDock"));
    _logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    _logDock->setTitleBarWidget(new DockTitleBar(_logDock, this));

    _logViewer = new QTextEdit(this);
    _logViewer->setObjectName(QStringLiteral("SystemLogViewer"));
    _logViewer->setReadOnly(true);
    _logViewer->document()->setMaximumBlockCount(500); // UI also rolls up to 500 lines

    _logDock->setWidget(_logViewer);
    addDockWidget(Qt::BottomDockWidgetArea, _logDock);
    installRecursiveEventFilter(_logDock, this);
}

void MainWindow::appendLog(const QString& msg) {
    if (_logViewer) {
        QScrollBar* bar = _logViewer->verticalScrollBar();
        // Check if the scrollbar was at the bottom before appending new log text.
        // We allow 10px margin for scrollbar padding/sub-pixel differences.
        bool wasAtBottom = (bar->value() + bar->pageStep() >= bar->maximum() - 10);

        _logViewer->append(msg);

        if (wasAtBottom) {
            bar->setValue(bar->maximum());
        }
    }
}

void MainWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        _dragStartPos = event->globalPosition().toPoint();
        QPoint localPos = mapFromGlobal(event->globalPosition().toPoint());
        int mode = determineResizeMode(localPos);
        if (mode != ResizeNone) {
            if (auto* window = this->windowHandle()) {
                Qt::Edges edges = Qt::Edges();
                if (mode & ResizeLeft) edges |= Qt::LeftEdge;
                if (mode & ResizeRight) edges |= Qt::RightEdge;
                if (mode & ResizeTop) edges |= Qt::TopEdge;
                if (mode & ResizeBottom) edges |= Qt::BottomEdge;

                window->startSystemResize(edges);
                event->accept();
                return;
            }
            _resizeMode = mode;
            event->accept();
            return;
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton && _resizeMode != ResizeNone) {
        QPoint currentGlobalPos = event->globalPosition().toPoint();
        QPoint delta = currentGlobalPos - _dragStartPos;
        QRect geom = geometry();

        int left = geom.left();
        int right = geom.x() + geom.width();
        int top = geom.top();
        int bottom = geom.y() + geom.height();

        if (_resizeMode & ResizeLeft) {
            left += delta.x();
            if (right - left < minimumWidth()) {
                left = right - minimumWidth();
            }
        }
        if (_resizeMode & ResizeRight) {
            right += delta.x();
            if (right - left < minimumWidth()) {
                right = left + minimumWidth();
            }
        }
        if (_resizeMode & ResizeTop) {
            top += delta.y();
            if (bottom - top < minimumHeight()) {
                top = bottom - minimumHeight();
            }
        }
        if (_resizeMode & ResizeBottom) {
            bottom += delta.y();
            if (bottom - top < minimumHeight()) {
                bottom = top + minimumHeight();
            }
        }

        setGeometry(left, top, right - left, bottom - top);
        _dragStartPos = currentGlobalPos;
        event->accept();
        return;
    } else {
        QPoint localPos = mapFromGlobal(event->globalPosition().toPoint());
        updateCursorShape(localPos);
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        _resizeMode = ResizeNone;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QMainWindow::mouseReleaseEvent(event);
    }
}

int MainWindow::determineResizeMode(const QPoint& pos)
{
    if (isMaximized()) {
        return ResizeNone;
    }

    int mode = ResizeNone;
    const int border = 6;

    if (pos.x() < border) mode |= ResizeLeft;
    if (pos.x() > width() - border) mode |= ResizeRight;
    if (pos.y() < border) mode |= ResizeTop;
    if (pos.y() > height() - border) mode |= ResizeBottom;

    return mode;
}

void MainWindow::updateCursorShape(const QPoint& pos)
{
    int mode = determineResizeMode(pos);

    if (mode == ResizeNone) {
        setCursor(Qt::ArrowCursor);
    } else if ((mode & ResizeLeft && mode & ResizeTop) || (mode & ResizeRight && mode & ResizeBottom)) {
        setCursor(Qt::SizeFDiagCursor);
    } else if ((mode & ResizeRight && mode & ResizeTop) || (mode & ResizeLeft && mode & ResizeBottom)) {
        setCursor(Qt::SizeBDiagCursor);
    } else if (mode & ResizeLeft || mode & ResizeRight) {
        setCursor(Qt::SizeHorCursor);
    } else if (mode & ResizeTop || mode & ResizeBottom) {
        setCursor(Qt::SizeVerCursor);
    }
}

void MainWindow::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    setCursor(Qt::ArrowCursor);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    auto* watchedWidget = qobject_cast<QWidget*>(watched);
    if (!watchedWidget) {
        return QMainWindow::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseMove) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint localPos = mapFromGlobal(mouseEvent->globalPosition().toPoint());

        if (_resizeMode != ResizeNone) {
            this->mouseMoveEvent(mouseEvent);
            return true;
        }

        int mode = determineResizeMode(localPos);
        if (mode != ResizeNone) {
            // Determine the cursor shape
            Qt::CursorShape shape = Qt::ArrowCursor;
            if ((mode & ResizeLeft && mode & ResizeTop) || (mode & ResizeRight && mode & ResizeBottom)) {
                shape = Qt::SizeFDiagCursor;
            } else if ((mode & ResizeRight && mode & ResizeTop) || (mode & ResizeLeft && mode & ResizeBottom)) {
                shape = Qt::SizeBDiagCursor;
            } else if (mode & ResizeLeft || mode & ResizeRight) {
                shape = Qt::SizeHorCursor;
            } else if (mode & ResizeTop || mode & ResizeBottom) {
                shape = Qt::SizeVerCursor;
            }

            this->setCursor(shape);
            watchedWidget->setCursor(shape);
            return true;
        } else {
            this->unsetCursor();
            watchedWidget->unsetCursor();
        }
    }
    else if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint localPos = mapFromGlobal(mouseEvent->globalPosition().toPoint());
        int mode = determineResizeMode(localPos);
        if (mode != ResizeNone) {
            this->mousePressEvent(mouseEvent);
            return true;
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease) {
        if (_resizeMode != ResizeNone) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            this->mouseReleaseEvent(mouseEvent);
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
