#pragma once
#include <QWidget>
#include <QProcess>
#include <QLibrary>

#ifdef HAS_OPENCV
#include <opencv2/opencv.hpp>
using ProcessFunc = void (*)(const cv::Mat&, cv::Mat&, double, double);
#else
using ProcessFunc = void*;
#endif

class QTextEdit;
class QPushButton;
class QSlider;
class QLabel;

class QProcessingWidget : public QWidget {
    Q_OBJECT
public:
    explicit QProcessingWidget(QWidget* parent = nullptr);
    ~QProcessingWidget() override;

    double param1() const;
    double param2() const;

signals:
    void filterReady(ProcessFunc filter);
    void parametersChanged(double p1, double p2);

private slots:
    void handleCompile();
    void handleCompilerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void updateLabels();

private:
    void initUI();
    void loadLibrary();
    void unloadLibrary();
    QString getScratchDir() const;

    QTextEdit* _codeEditor = nullptr;
    QPushButton* _compileButton = nullptr;
    QSlider* _slider1 = nullptr;
    QSlider* _slider2 = nullptr;
    QLabel* _slider1Label = nullptr;
    QLabel* _slider2Label = nullptr;
    QTextEdit* _logConsole = nullptr;

    QProcess* _compiler = nullptr;
    QLibrary* _library = nullptr;
    ProcessFunc _filterFunc = nullptr;
};
