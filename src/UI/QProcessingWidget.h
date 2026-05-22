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

class QProcessingWidget : public QWidget {
    Q_OBJECT
public:
    explicit QProcessingWidget(QWidget* parent = nullptr);
    ~QProcessingWidget() override;

    void setController(AbstractImagingController* controller);

private slots:
    void handleAddNode();
    void handleRemoveNode();
    void handleMoveUp();
    void handleMoveDown();
    void handleNodeItemChanged(QListWidgetItem* item);
    void handlePipelineSelectionChanged();
    void handleSliderValueChanged();

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
    QListWidget* _availableList = nullptr;
    QListWidget* _pipelineList = nullptr;

    QPushButton* _addButton = nullptr;
    QPushButton* _removeButton = nullptr;
    QPushButton* _moveUpButton = nullptr;
    QPushButton* _moveDownButton = nullptr;

    QSlider* _slider1 = nullptr;
    QSlider* _slider2 = nullptr;
    QLabel* _slider1Label = nullptr;
    QLabel* _slider2Label = nullptr;
    QLabel* _selectedNodeLabel = nullptr;

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
