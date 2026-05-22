#pragma once
#include <QMainWindow>
#include <memory>

class QMdiArea;
class CameraSystem;
class QTextEdit;
class QDockWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onAddBaslerCamera();
    void onAddLmiGocator();
    void appendLog(const QString& msg);

private:
    void createMenus();
    void createLogDock();

    QMdiArea* _mdiArea;
    QTextEdit* _logViewer = nullptr;
    QDockWidget* _logDock = nullptr;
    std::unique_ptr<CameraSystem> _cameraSystem;
};
