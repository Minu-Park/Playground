#pragma once
#include <QWidget>
#include <QProcess>
#include <memory>
#include <vector>

class QPushButton;
class QSlider;
class QLabel;
class QTextEdit;
class QCodeEditor;
class AbstractImagingController;
class ProcessingNode;
class DynamicLibraryLoader;
class QBoxLayout;
class QResizeEvent;
class QToolButton;
class QVBoxLayout;
class DynamicWrapperNode;

class QProcessingWidget : public QWidget {
    Q_OBJECT
public:
    explicit QProcessingWidget(QWidget* parent = nullptr);
    ~QProcessingWidget() override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setController(AbstractImagingController* controller);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void handleDynamicSliderChanged();
    void handleDynamicSpinBoxChanged(double value);

    // Compiler slots
    void handleCompile();
    void handleCompilerFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void initUI();
    void refreshAvailableNodes();
    void updatePipelineInController();
    void loadDynamicNode(const QString& dylibPath);
    QString getScratchDir() const;

    AbstractImagingController* _controller = nullptr;

    void refreshParameterUI();

    // UI components
    QBoxLayout* _mainLayout = nullptr;
    QToolButton* _enableToggleBtn = nullptr;

    // Dynamic Parameter UI Tracking
    struct ParameterControl {
        int index;
        QLabel* nameLabel = nullptr;
        QLabel* valueLabel = nullptr;
        QWidget* controlWidget = nullptr; // Can be QSlider (int) or QDoubleSpinBox (double)
    };

    QVBoxLayout* _paramLayout = nullptr;
    std::vector<ParameterControl> _dynamicControls;

    // Dynamic compiler UI (OpenCV optional)
    QCodeEditor* _codeEditor = nullptr;
    QPushButton* _compileButton = nullptr;

    QProcess* _compiler = nullptr;

    // Script node instance
    std::shared_ptr<DynamicWrapperNode> _scriptNode;

    // Loaded dynamic library loader (kept alive so library memory isn't unmapped prematurely)
    std::shared_ptr<DynamicLibraryLoader> _dynamicLoader;
    QString _currentDylibPath;
};
