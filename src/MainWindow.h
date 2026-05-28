#pragma once
#include <QMainWindow>
#include <memory>

class QMdiArea;
class CameraSystem;
class QTextEdit;
class QDockWidget;
class QEvent;

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
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void createMenus();
    void createLogDock();
    void createOpenGLCompositionSeed();

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
    std::unique_ptr<CameraSystem> _cameraSystem;

    QPoint _dragStartPos;
    int _resizeMode = ResizeNone;
};
