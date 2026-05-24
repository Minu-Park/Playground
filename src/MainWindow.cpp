#include "MainWindow.h"
#include "DeviceSession.h"
#include "CameraSystem.h"
#include "Gocator.h"
#include "Utility/LogManager.h"
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QIcon>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QDockWidget>
#include <QTextEdit>
#include <QScrollBar>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QFileDialog>

namespace {

class BrandedMdiArea final : public QMdiArea {
public:
    explicit BrandedMdiArea(QWidget* parent = nullptr)
        : QMdiArea(parent)
        , _logo(QStringLiteral(":/Resources/BASLER_Logo.png"))
    {}

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(viewport());
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

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Playground (MDI Workspace)"));
    resize(1600, 900);

    _cameraSystem = std::make_unique<CameraSystem>();

    _mdiArea = new BrandedMdiArea(this);
    _mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(_mdiArea);

    createLogDock();
    createMenus();

    // Connect global logger to our log dock using QueuedConnection for thread-safety.
    connect(LogManager::instance(), &LogManager::logAdded, this, &MainWindow::appendLog, Qt::QueuedConnection);
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
    connect(tileAct, &QAction::triggered, _mdiArea, &QMdiArea::tileSubWindows);
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
    _mdiArea->addSubWindow(session);
    session->show();
}

void MainWindow::onAddLmiGocator() {
    auto* gocator = new Gocator();
    auto* session = new DeviceSession(gocator, nullptr);
    session->setAttribute(Qt::WA_DeleteOnClose);
    session->setWindowTitle(QStringLiteral("LMI Gocator Session"));
    _mdiArea->addSubWindow(session);
    session->show();
}

void MainWindow::onAddTestImageSession() {
    auto* session = new DeviceSession(QStringList(), nullptr);
    session->setAttribute(Qt::WA_DeleteOnClose);
    session->setWindowTitle(QStringLiteral("Test Images Session"));
    _mdiArea->addSubWindow(session);
    session->show();
}

void MainWindow::createLogDock() {
    _logDock = new QDockWidget(tr("System Logs"), this);
    _logDock->setObjectName(QStringLiteral("SystemLogsDock"));
    _logDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    _logViewer = new QTextEdit(this);
    _logViewer->setReadOnly(true);
    _logViewer->document()->setMaximumBlockCount(500); // UI also rolls up to 500 lines

    // Style the text edit to align with the clean white design system
    _logViewer->setStyleSheet(QStringLiteral(
        "QTextEdit {"
        "  background-color: #ffffff;"
        "  color: #16202b;"
        "  border: 1px solid #d9e1ea;"
        "  border-radius: 8px;"
        "  font-family: 'Menlo', 'Monaco', 'Consolas', 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New', monospace;"
        "  font-size: 11px;"
        "  padding: 8px;"
        "}"
    ));

    _logDock->setWidget(_logViewer);
    addDockWidget(Qt::BottomDockWidgetArea, _logDock);
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
