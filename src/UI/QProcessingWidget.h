#pragma once
#include <QWidget>
#include <QProcess>
#include <QString>
#include <memory>
#include <vector>

class QPushButton;
class QLabel;
class QCodeEditor;
class QTextEdit;
class AbstractImagingController;
class QBoxLayout;
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

private slots:
    void handleDynamicSliderChanged();
    void handleDynamicSpinBoxChanged(double value);

    // Compiler slots
    void handleCompile();
    void handleCompilerFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void initUI();
    void updatePipelineInController();
    void loadDynamicNode(const QString& dylibPath);
    QString getScratchDir() const;
    QString getAutosaveDir() const;

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

    // Parameter definition editor
    QTextEdit* _paramEditor = nullptr;

    // Dynamic compiler UI (OpenCV optional)
    QCodeEditor* _codeEditor = nullptr;
    QToolButton* _runtimePathsButton = nullptr;
    QToolButton* _compileButton = nullptr;

    QProcess* _compiler = nullptr;
    QString _scratchDir;
    QString _autosaveDir;

    // Script node instance
    std::shared_ptr<DynamicWrapperNode> _scriptNode;
};
