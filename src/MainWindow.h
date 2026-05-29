#pragma once
#include <QMainWindow>
#include <QVector>
#include <QMap>
#include <memory>

class QMdiArea;
class QMdiSubWindow;
class CameraSystem;
class QTextEdit;
class QDockWidget;
class QEvent;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onAddBaslerCamera();
    void onAddLmiGocator();
    void onAddTestImageSession();
    void onTileWindows();
    void appendLog(const QString& msg);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void createMenus();
    void createLogDock();
    void createOpenGLCompositionSeed();
    void createMainStatusBar();
    void updateMainStatusBar();
    void refreshPluginIndicators();
    void minimizeSession(QMdiSubWindow* subWin);
    void restoreSession(QMdiSubWindow* subWin);
    void closeMinimizedSession(QMdiSubWindow* subWin);

    enum ResizeDirection {
        ResizeNone = 0,
        ResizeLeft = 1,
        ResizeRight = 2,
        ResizeTop = 4,
        ResizeBottom = 8
    };

    int determineResizeMode(const QPoint& pos);
    void updateCursorShape(const QPoint& pos);

    QMdiArea* _mdiArea;
    QTextEdit* _logViewer = nullptr;
    QDockWidget* _logDock = nullptr;
    QLabel* _sessionCountStatus = nullptr;
    QLabel* _activeSessionStatus = nullptr;
    QVector<QLabel*> _pluginIndicators;
    QMap<QMdiSubWindow*, QWidget*> _minimizedIndicators;
    std::unique_ptr<CameraSystem> _cameraSystem;

    QPoint _dragStartPos;
    int _resizeMode = ResizeNone;
};
