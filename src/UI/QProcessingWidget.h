#pragma once
#include <QWidget>
#include <QProcess>
#include <memory>
#include <vector>

class QListWidget;
class QPushButton;
class QSlider;
class QLabel;
class QTextEdit;
class QListWidgetItem;
class AbstractImagingController;
class ProcessingNode;
class DynamicLibraryLoader;
class QBoxLayout;
class QResizeEvent;
class QTabWidget;
class QToolButton;
class QVBoxLayout;

class QProcessingWidget : public QWidget {
    Q_OBJECT
public:
    explicit QProcessingWidget(QWidget* parent = nullptr);
    ~QProcessingWidget() override;

    void setController(AbstractImagingController* controller);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void handleAddNode();
    void handleRemoveNode();
    void handleMoveUp();
    void handleMoveDown();
    void handleNodeItemChanged(QListWidgetItem* item);
    void handlePipelineSelectionChanged();
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

    // UI components
    QBoxLayout* _mainLayout = nullptr;
    QTabWidget* _tabWidget = nullptr;
    QListWidget* _availableList = nullptr;
    QListWidget* _pipelineList = nullptr;

    QToolButton* _addButton = nullptr;
    QToolButton* _removeButton = nullptr;
    QToolButton* _moveUpButton = nullptr;
    QToolButton* _moveDownButton = nullptr;

    // Dynamic Parameter UI Tracking
    struct ParameterControl {
        int index;
        QLabel* nameLabel = nullptr;
        QLabel* valueLabel = nullptr;
        QWidget* controlWidget = nullptr; // Can be QSlider (int) or QDoubleSpinBox (double)
    };

    QLabel* _selectedNodeLabel = nullptr;
    QVBoxLayout* _paramLayout = nullptr;
    std::vector<ParameterControl> _dynamicControls;

    // Dynamic compiler UI (OpenCV optional)
    QTextEdit* _codeEditor = nullptr;
    QPushButton* _compileButton = nullptr;
    QTextEdit* _logConsole = nullptr;

    QProcess* _compiler = nullptr;

    // Active nodes currently in the pipeline (managed by UI)
    std::vector<std::shared_ptr<ProcessingNode>> _activeNodes;

    // Loaded dynamic library loader (kept alive so library memory isn't unmapped prematurely)
    std::shared_ptr<DynamicLibraryLoader> _dynamicLoader;
};
