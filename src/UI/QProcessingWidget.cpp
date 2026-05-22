#include "UI/QProcessingWidget.h"
#include "Controller/AbstractImagingController.h"
#include "Pipeline/ProcessingRegistry.h"
#include "Pipeline/DynamicLibraryLoader.h"
#include "Pipeline/ProcessingPipeline.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>
#include <QPalette>
#include <QFont>
#include <QDebug>

QProcessingWidget::QProcessingWidget(QWidget* parent)
    : QWidget(parent) {
    initUI();
    refreshAvailableNodes();
}

QProcessingWidget::~QProcessingWidget() {
    if (_compiler) {
        _compiler->kill();
        _compiler->waitForFinished();
    }
}

void QProcessingWidget::setController(AbstractImagingController* controller) {
    _controller = controller;
    if (_controller) {
        updatePipelineInController();
    }
}

void QProcessingWidget::initUI() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    // Left side: Pipeline Manager (Available & Active Pipeline)
    auto* leftLayout = new QVBoxLayout();

    auto* availLabel = new QLabel(QStringLiteral("Available Nodes:"), this);
    _availableList = new QListWidget(this);

    auto* pipelineLabel = new QLabel(QStringLiteral("Pipeline Nodes (Check to enable):"), this);
    _pipelineList = new QListWidget(this);

    // Connect list widget change to update controller
    connect(_pipelineList, &QListWidget::itemChanged, this, &QProcessingWidget::handleNodeItemChanged);
    connect(_pipelineList, &QListWidget::itemSelectionChanged, this, &QProcessingWidget::handlePipelineSelectionChanged);

    // Buttons Layout
    auto* btnLayout = new QHBoxLayout();
    _addButton = new QPushButton(QStringLiteral("Add Node"), this);
    _removeButton = new QPushButton(QStringLiteral("Remove"), this);
    _moveUpButton = new QPushButton(QStringLiteral("Up"), this);
    _moveDownButton = new QPushButton(QStringLiteral("Down"), this);

    btnLayout->addWidget(_addButton);
    btnLayout->addWidget(_removeButton);
    btnLayout->addWidget(_moveUpButton);
    btnLayout->addWidget(_moveDownButton);

    connect(_addButton, &QPushButton::clicked, this, &QProcessingWidget::handleAddNode);
    connect(_removeButton, &QPushButton::clicked, this, &QProcessingWidget::handleRemoveNode);
    connect(_moveUpButton, &QPushButton::clicked, this, &QProcessingWidget::handleMoveUp);
    connect(_moveDownButton, &QPushButton::clicked, this, &QProcessingWidget::handleMoveDown);

    leftLayout->addWidget(availLabel);
    leftLayout->addWidget(_availableList, 1);
    leftLayout->addWidget(pipelineLabel);
    leftLayout->addWidget(_pipelineList, 2);
    leftLayout->addLayout(btnLayout);

    mainLayout->addLayout(leftLayout, 3);

    // Right side: Settings & Dynamic Compiler
    auto* rightLayout = new QVBoxLayout();

    // Node Parameter Editor Box
    auto* paramLayout = new QVBoxLayout();
    _selectedNodeLabel = new QLabel(QStringLiteral("Selected Node: None"), this);
    _selectedNodeLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px; color: #007acc;"));

    auto* s1Header = new QHBoxLayout();
    auto* s1Label = new QLabel(QStringLiteral("Parameter 1 (Blur size / Param A):"), this);
    _slider1Label = new QLabel(QStringLiteral("0"), this);
    s1Header->addWidget(s1Label);
    s1Header->addWidget(_slider1Label, 0, Qt::AlignRight);

    _slider1 = new QSlider(Qt::Horizontal, this);
    _slider1->setRange(1, 100);
    _slider1->setValue(5);

    auto* s2Header = new QHBoxLayout();
    auto* s2Label = new QLabel(QStringLiteral("Parameter 2 (Canny Threshold / Param B):"), this);
    _slider2Label = new QLabel(QStringLiteral("0"), this);
    s2Header->addWidget(s2Label);
    s2Header->addWidget(_slider2Label, 0, Qt::AlignRight);

    _slider2 = new QSlider(Qt::Horizontal, this);
    _slider2->setRange(1, 250);
    _slider2->setValue(50);

    paramLayout->addWidget(_selectedNodeLabel);
    paramLayout->addLayout(s1Header);
    paramLayout->addWidget(_slider1);
    paramLayout->addLayout(s2Header);
    paramLayout->addWidget(_slider2);

    connect(_slider1, &QSlider::valueChanged, this, &QProcessingWidget::handleSliderValueChanged);
    connect(_slider2, &QSlider::valueChanged, this, &QProcessingWidget::handleSliderValueChanged);

    rightLayout->addLayout(paramLayout);

    // Logger & Code editor (OpenCV conditional compile)
    QFont monoFont(QStringLiteral("Courier New"), 12);
    monoFont.setStyleHint(QFont::Monospace);

    auto* logTitle = new QLabel(QStringLiteral("Compilation & Load Console:"), this);
    _logConsole = new QTextEdit(this);
    _logConsole->setReadOnly(true);
    _logConsole->setFont(monoFont);

    QPalette p = _logConsole->palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::green);
    _logConsole->setPalette(p);

#ifndef HAS_OPENCV
    auto* noOpenCVLabel = new QLabel(
        QStringLiteral("OpenCV is not enabled. C++ live compilation is disabled.\n"
                       "Using built-in processing registry."), this);
    noOpenCVLabel->setStyleSheet(QStringLiteral("color: #cc6600; font-weight: bold;"));
    rightLayout->addWidget(noOpenCVLabel);
    _logConsole->append(QStringLiteral("[System] Pipeline active in OpenCV-disabled mode."));
#else
    auto* editorTitle = new QLabel(QStringLiteral("Dynamic C++ OpenCV Filter Node (C++20):"), this);
    _codeEditor = new QTextEdit(this);
    _codeEditor->setFont(monoFont);

    // Prepopulate with updated modern template using the Qt-independent C contract
    QString templateCode =
        "#include <opencv2/opencv.hpp>\n\n"
        "extern \"C\" {\n"
        "    // OpenCV dynamic filter implementation (Qt-independent)\n"
        "    void process_image(unsigned char* data, int width, int height, int channels, int step, double param1, double param2) {\n"
        "        // Create cv::Mat wrapping raw pixels\n"
        "        int type = (channels == 4) ? CV_8UC4 : ((channels == 3) ? CV_8UC3 : CV_8UC1);\n"
        "        cv::Mat mat(height, width, type, data, step);\n\n"
        "        cv::Mat gray, blurred, edges;\n"
        "        if (channels == 4) {\n"
        "            cv::cvtColor(mat, gray, cv::COLOR_BGRA2GRAY);\n"
        "        } else if (channels == 3) {\n"
        "            cv::cvtColor(mat, gray, cv::COLOR_BGR2GRAY);\n"
        "        } else {\n"
        "            gray = mat;\n"
        "        }\n\n"
        "        // param1 controls Gaussian blur kernel size (must be odd)\n"
        "        int kernelSize = (int(param1) % 2 == 0) ? int(param1) + 1 : int(param1);\n"
        "        kernelSize = std::max(1, kernelSize);\n\n"
        "        cv::GaussianBlur(gray, blurred, cv::Size(kernelSize, kernelSize), 0);\n"
        "        // param2 controls Canny low threshold\n"
        "        cv::Canny(blurred, edges, param2, param2 * 3);\n\n"
        "        // Convert edge grayscale result back to the output buffer\n"
        "        // Since we modify in-place, we must convert grayscale back to target channels\n"
        "        if (channels == 4) {\n"
        "            cv::cvtColor(edges, mat, cv::COLOR_GRAY2BGRA);\n"
        "        } else if (channels == 3) {\n"
        "            cv::cvtColor(edges, mat, cv::COLOR_GRAY2BGR);\n"
        "        } else {\n"
        "            edges.copyTo(mat);\n"
        "        }\n"
        "    }\n"
        "}\n";
    _codeEditor->setPlainText(templateCode);

    _compileButton = new QPushButton(QStringLiteral("Compile & Apply Hot-Swap"), this);
    _compileButton->setStyleSheet(QStringLiteral("background-color: #007acc; color: white; font-weight: bold; padding: 6px;"));

    connect(_compileButton, &QPushButton::clicked, this, &QProcessingWidget::handleCompile);

    rightLayout->addWidget(editorTitle);
    rightLayout->addWidget(_codeEditor, 3);
    rightLayout->addWidget(_compileButton);
#endif

    rightLayout->addWidget(logTitle);
    rightLayout->addWidget(_logConsole, 1);
    mainLayout->addLayout(rightLayout, 4);

    handlePipelineSelectionChanged(); // Initial refresh of slider state
}

void QProcessingWidget::refreshAvailableNodes() {
    if (!_availableList) return;
    _availableList->clear();
    auto list = ProcessingRegistry::instance().availableNodes();
    for (const auto& def : list) {
        auto* item = new QListWidgetItem(_availableList);
        item->setText(QStringLiteral("%1 (%2)").arg(def.displayName, def.id));
        item->setData(Qt::UserRole, def.id);
    }
}

void QProcessingWidget::handleAddNode() {
    auto* item = _availableList->currentItem();
    if (!item) return;

    QString id = item->data(Qt::UserRole).toString();
    auto node = ProcessingRegistry::instance().createNode(id);
    if (node) {
        _activeNodes.push_back(node);

        auto* pipeItem = new QListWidgetItem(_pipelineList);
        pipeItem->setText(node->name());
        pipeItem->setFlags(pipeItem->flags() | Qt::ItemIsUserCheckable);
        pipeItem->setCheckState(Qt::Checked);

        updatePipelineInController();
    }
}

void QProcessingWidget::handleRemoveNode() {
    int index = _pipelineList->currentRow();
    if (index < 0 || index >= (int)_activeNodes.size()) return;

    _activeNodes.erase(_activeNodes.begin() + index);
    delete _pipelineList->takeItem(index);

    updatePipelineInController();
    handlePipelineSelectionChanged();
}

void QProcessingWidget::handleMoveUp() {
    int index = _pipelineList->currentRow();
    if (index <= 0 || index >= (int)_activeNodes.size()) return;

    std::swap(_activeNodes[index], _activeNodes[index - 1]);

    auto* item = _pipelineList->takeItem(index);
    _pipelineList->insertItem(index - 1, item);
    _pipelineList->setCurrentRow(index - 1);

    updatePipelineInController();
}

void QProcessingWidget::handleMoveDown() {
    int index = _pipelineList->currentRow();
    if (index < 0 || index >= (int)_activeNodes.size() - 1) return;

    std::swap(_activeNodes[index], _activeNodes[index + 1]);

    auto* item = _pipelineList->takeItem(index);
    _pipelineList->insertItem(index + 1, item);
    _pipelineList->setCurrentRow(index + 1);

    updatePipelineInController();
}

void QProcessingWidget::handleNodeItemChanged(QListWidgetItem* item) {
    int index = _pipelineList->row(item);
    if (index < 0 || index >= (int)_activeNodes.size()) return;

    bool isChecked = (item->checkState() == Qt::Checked);
    _activeNodes[index]->setEnabled(isChecked);

    updatePipelineInController();
}

void QProcessingWidget::handlePipelineSelectionChanged() {
    int index = _pipelineList->currentRow();
    if (index < 0 || index >= (int)_activeNodes.size()) {
        _selectedNodeLabel->setText(QStringLiteral("Selected Node: None"));
        _slider1->setEnabled(false);
        _slider2->setEnabled(false);
        return;
    }

    auto& node = _activeNodes[index];
    _selectedNodeLabel->setText(QStringLiteral("Selected Node: %1").arg(node->name()));
    _slider1->setEnabled(true);
    _slider2->setEnabled(true);

    _slider1Label->setText(QString::number(_slider1->value()));
    _slider2Label->setText(QString::number(_slider2->value()));
}

void QProcessingWidget::handleSliderValueChanged() {
    int index = _pipelineList->currentRow();
    if (index < 0 || index >= (int)_activeNodes.size()) return;

    auto& node = _activeNodes[index];
    double val1 = static_cast<double>(_slider1->value());
    double val2 = static_cast<double>(_slider2->value());

    _slider1Label->setText(QString::number(_slider1->value()));
    _slider2Label->setText(QString::number(_slider2->value()));

    node->setParameter(0, val1);
    node->setParameter(1, val2);
}

void QProcessingWidget::updatePipelineInController() {
    if (!_controller) return;
    auto* pipe = _controller->pipeline();
    if (!pipe) return;

    pipe->clearNodes();
    for (const auto& node : _activeNodes) {
        pipe->addNode(node);
    }
}

QString QProcessingWidget::getScratchDir() const {
    return QCoreApplication::applicationDirPath() + QStringLiteral("/scratch");
}

void QProcessingWidget::handleCompile() {
#ifdef HAS_OPENCV
    if (!_codeEditor || !_compileButton || !_logConsole) return;

    _compileButton->setEnabled(false);
    _compileButton->setText(QStringLiteral("Compiling..."));
    _logConsole->clear();
    _logConsole->append(QStringLiteral("Saving dynamic filter script..."));

    QString scratchDir = getScratchDir();
    QDir().mkpath(scratchDir);

    QString filePath = scratchDir + QStringLiteral("/dynamic_filter.cpp");
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        _logConsole->append(QStringLiteral("Error: Failed to create dynamic_filter.cpp"));
        _compileButton->setEnabled(true);
        _compileButton->setText(QStringLiteral("Compile & Apply Hot-Swap"));
        return;
    }

    QTextStream out(&file);
    out << _codeEditor->toPlainText();
    file.close();

    _logConsole->append(QStringLiteral("Launching clang++..."));

    if (_compiler) {
        _compiler->kill();
        _compiler->deleteLater();
    }

    _compiler = new QProcess(this);
    _compiler->setWorkingDirectory(scratchDir);

    QStringList arguments;
    arguments << QStringLiteral("-O2")
              << QStringLiteral("-shared")
              << QStringLiteral("-fPIC")
              << QStringLiteral("-std=c++20");

    // Add OpenCV headers
    QString incDir = QString::fromLocal8Bit(OPENCV_INCLUDE_DIR);
    arguments << QStringLiteral("-I") + incDir;

    // Add project src directory so compiled node can include ProcessingPipeline.h
    arguments << QStringLiteral("-I") + QStringLiteral(PLAYGROUND_SRC_DIR);

    // Add OpenCV library path
    QString libDir = QString::fromLocal8Bit(OPENCV_LIB_DIR);
    arguments << QStringLiteral("-L") + libDir;
    arguments << QStringLiteral("-lopencv_core") << QStringLiteral("-lopencv_imgproc");

    // Input / Output files
    arguments << QStringLiteral("dynamic_filter.cpp")
              << QStringLiteral("-o")
              << QStringLiteral("libdynamic_filter.dylib");

    _logConsole->append(QStringLiteral("Command: clang++ ") + arguments.join(QStringLiteral(" ")));

    connect(_compiler, &QProcess::finished, this, &QProcessingWidget::handleCompilerFinished);
    _compiler->start(QStringLiteral("clang++"), arguments);
#endif
}

void QProcessingWidget::handleCompilerFinished(int exitCode, QProcess::ExitStatus exitStatus) {
#ifdef HAS_OPENCV
    _compileButton->setEnabled(true);
    _compileButton->setText(QStringLiteral("Compile & Apply Hot-Swap"));

    if (exitStatus == QProcess::CrashExit) {
        _logConsole->append(QStringLiteral("\n[Error] Compiler process crashed!"));
        return;
    }

    if (exitCode == 0) {
        _logConsole->append(QStringLiteral("\n[Success] Compilation succeeded!"));
        QString dylibPath = getScratchDir() + QStringLiteral("/libdynamic_filter.dylib");
        loadDynamicNode(dylibPath);
    } else {
        QString errStr = QString::fromLocal8Bit(_compiler->readAllStandardError());
        _logConsole->append(QStringLiteral("\n[Failed] Compilation failed with code ") + QString::number(exitCode));
        _logConsole->append(errStr);
    }
#endif
}

void QProcessingWidget::loadDynamicNode(const QString& dylibPath) {
    _logConsole->append(QStringLiteral("Loading dylib: ") + dylibPath);

    // 1. Load the dynamic library loader
    auto loader = DynamicLibraryLoader::load(dylibPath);
    if (!loader) {
        _logConsole->append(QStringLiteral("[Error] Failed to load library."));
        return;
    }

    // 2. Create the node
    // Since we decided to use a Qt-independent symbol mapping for maximum compiler robustness,
    // let's resolve 'process_image' instead of 'create_node' if we use the simple contract.
    // Let's implement a wrapper node that calls 'process_image'.
    using ProcessImageFunc = void (*)(unsigned char*, int, int, int, int, double, double);
    auto func = reinterpret_cast<ProcessImageFunc>(loader->resolve("process_image"));
    if (!func) {
        _logConsole->append(QStringLiteral("[Error] Symbol 'process_image' not resolved."));
        return;
    }

    // Custom inline wrapper node that holds std::shared_ptr<DynamicLibraryLoader>
    class DynamicWrapperNode : public ProcessingNode {
    public:
        DynamicWrapperNode(std::shared_ptr<DynamicLibraryLoader> l, ProcessImageFunc f)
            : _loader(std::move(l)), _func(f) {}

        QString name() const override { return "Dynamic Hot-Swap Filter"; }

        void setParameter(int index, double value) override {
            if (index == 0) _p1 = value;
            else if (index == 1) _p2 = value;
        }

        void process(ProcessingFrame& frame) override {
            if (std::holds_alternative<QImage>(frame.payload)) {
                QImage& img = std::get<QImage>(frame.payload);
                if (img.isNull()) return;

                // Deep copy if shared, to guarantee thread-safe in-place modification
                img.detach();

                int channels = 1;
                if (img.format() == QImage::Format_RGB32 || img.format() == QImage::Format_ARGB32) {
                    channels = 4;
                } else if (img.format() == QImage::Format_RGB888 || img.format() == QImage::Format_BGR888) {
                    channels = 3;
                }

                _func(img.bits(), img.width(), img.height(), channels, img.bytesPerLine(), _p1, _p2);
            }
        }

    private:
        std::shared_ptr<DynamicLibraryLoader> _loader;
        ProcessImageFunc _func = nullptr;
        double _p1 = 5.0;
        double _p2 = 50.0;
    };

    auto dynamicNode = std::make_shared<DynamicWrapperNode>(loader, func);

    // Set parameters from current sliders
    dynamicNode->setParameter(0, static_cast<double>(_slider1->value()));
    dynamicNode->setParameter(1, static_cast<double>(_slider2->value()));

    // 3. Keep the loader alive globally in UI too (so we have a reference, though the node also has one)
    _dynamicLoader = loader;

    // 4. Remove any existing dynamic wrapper nodes in our pipeline
    for (auto it = _activeNodes.begin(); it != _activeNodes.end(); ) {
        if ((*it)->name() == QStringLiteral("Dynamic Hot-Swap Filter")) {
            // Find corresponding item in list widget and remove
            int idx = std::distance(_activeNodes.begin(), it);
            _activeNodes.erase(it);
            delete _pipelineList->takeItem(idx);
            // Don't break because there could theoretically be more, but we just continue
        } else {
            ++it;
        }
    }

    // 5. Add new dynamic node
    _activeNodes.push_back(dynamicNode);
    auto* item = new QListWidgetItem(_pipelineList);
    item->setText(dynamicNode->name());
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Checked);

    _logConsole->append(QStringLiteral("[OK] Dynamic Hot-Swap Filter active and injected."));

    updatePipelineInController();
    handlePipelineSelectionChanged();
}
