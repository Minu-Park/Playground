#include "QProcessingWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>
#include <QPalette>
#include <QFont>

QProcessingWidget::QProcessingWidget(QWidget* parent)
    : QWidget(parent) {
    initUI();
}

QProcessingWidget::~QProcessingWidget() {
    unloadLibrary();
}

double QProcessingWidget::param1() const {
    return _slider1 ? static_cast<double>(_slider1->value()) : 0.0;
}

double QProcessingWidget::param2() const {
    return _slider2 ? static_cast<double>(_slider2->value()) : 0.0;
}

void QProcessingWidget::initUI() {
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

#ifndef HAS_OPENCV
    auto* noOpenCVLabel = new QLabel(
        QStringLiteral("OpenCV was not found. Live C++ compile panel is disabled.\n"
                       "Please install OpenCV and reconfigure CMake to enable this feature."),
        this);
    noOpenCVLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(noOpenCVLabel);
    return;
#else
    // 1. Left side: Code Editor
    auto* leftLayout = new QVBoxLayout();
    auto* editorTitle = new QLabel(QStringLiteral("C++ OpenCV Filter Script:"), this);
    _codeEditor = new QTextEdit(this);
    
    // Set fixed width font for editor
    QFont monoFont(QStringLiteral("Courier New"), 12);
    monoFont.setStyleHint(QFont::Monospace);
    _codeEditor->setFont(monoFont);
    
    // Load default template code
    QString templateCode = 
        "#include <opencv2/opencv.hpp>\n\n"
        "extern \"C\" {\n"
        "    void processImage(const cv::Mat& src, cv::Mat& dst, double param1, double param2) {\n"
        "        // Default pipeline: Gaussian Blur and Canny Edge Detection\n"
        "        cv::Mat gray, blurred;\n"
        "        if (src.channels() == 3) {\n"
        "            cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);\n"
        "        } else {\n"
        "            gray = src;\n"
        "        }\n\n"
        "        // param1 controls Gaussian blur kernel size (must be odd)\n"
        "        int kernelSize = (int(param1) % 2 == 0) ? int(param1) + 1 : int(param1);\n"
        "        kernelSize = std::max(1, kernelSize);\n\n"
        "        cv::GaussianBlur(gray, blurred, cv::Size(kernelSize, kernelSize), 0);\n"
        "        // param2 controls Canny low threshold\n"
        "        cv::Canny(blurred, dst, param2, param2 * 3);\n"
        "    }\n"
        "}\n";
    _codeEditor->setPlainText(templateCode);
    
    leftLayout->addWidget(editorTitle);
    leftLayout->addWidget(_codeEditor, 3); // Spawns 3/5 width
    mainLayout->addLayout(leftLayout, 3);

    // 2. Right side: Controls and Logger
    auto* rightLayout = new QVBoxLayout();
    
    // Sliders
    auto* slidersLayout = new QVBoxLayout();
    
    auto* s1Header = new QHBoxLayout();
    auto* s1Label = new QLabel(QStringLiteral("Parameter 1 (Blur size):"), this);
    _slider1Label = new QLabel(QStringLiteral("5"), this);
    s1Header->addWidget(s1Label);
    s1Header->addWidget(_slider1Label, 0, Qt::AlignRight);
    
    _slider1 = new QSlider(Qt::Horizontal, this);
    _slider1->setRange(1, 100);
    _slider1->setValue(5);
    
    auto* s2Header = new QHBoxLayout();
    auto* s2Label = new QLabel(QStringLiteral("Parameter 2 (Canny Threshold):"), this);
    _slider2Label = new QLabel(QStringLiteral("50"), this);
    s2Header->addWidget(s2Label);
    s2Header->addWidget(_slider2Label, 0, Qt::AlignRight);
    
    _slider2 = new QSlider(Qt::Horizontal, this);
    _slider2->setRange(1, 250);
    _slider2->setValue(50);
    
    slidersLayout->addLayout(s1Header);
    slidersLayout->addWidget(_slider1);
    slidersLayout->addLayout(s2Header);
    slidersLayout->addWidget(_slider2);
    
    // Compile Button
    _compileButton = new QPushButton(QStringLiteral("Compile & Apply"), this);
    _compileButton->setStyleSheet(QStringLiteral("background-color: #007acc; color: white; font-weight: bold; padding: 6px;"));

    // Logger
    auto* logTitle = new QLabel(QStringLiteral("Compilation Console:"), this);
    _logConsole = new QTextEdit(this);
    _logConsole->setReadOnly(true);
    _logConsole->setFont(monoFont);
    
    QPalette p = _logConsole->palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::green);
    _logConsole->setPalette(p);
    _logConsole->append(QStringLiteral("Ready. Click 'Compile & Apply' to load dynamic filter."));

    rightLayout->addLayout(slidersLayout);
    rightLayout->addWidget(_compileButton);
    rightLayout->addWidget(logTitle);
    rightLayout->addWidget(_logConsole, 1);
    mainLayout->addLayout(rightLayout, 2);

    // Connect sliders
    connect(_slider1, &QSlider::valueChanged, this, &QProcessingWidget::updateLabels);
    connect(_slider2, &QSlider::valueChanged, this, &QProcessingWidget::updateLabels);
    
    // Connect compiler button
    connect(_compileButton, &QPushButton::clicked, this, &QProcessingWidget::handleCompile);
#endif
}

void QProcessingWidget::updateLabels() {
    if (!_slider1 || !_slider2) return;
    _slider1Label->setText(QString::number(_slider1->value()));
    _slider2Label->setText(QString::number(_slider2->value()));
    emit parametersChanged(static_cast<double>(_slider1->value()), static_cast<double>(_slider2->value()));
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
    _logConsole->append(QStringLiteral("Saving script file..."));

    // Create scratch directory
    QString scratchDir = getScratchDir();
    QDir().mkpath(scratchDir);

    // Save current script code
    QString filePath = scratchDir + QStringLiteral("/dynamic_filter.cpp");
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        _logConsole->append(QStringLiteral("Error: Failed to create dynamic_filter.cpp"));
        _compileButton->setEnabled(true);
        _compileButton->setText(QStringLiteral("Compile & Apply"));
        return;
    }
    
    QTextStream out(&file);
    out << _codeEditor->toPlainText();
    file.close();

    _logConsole->append(QStringLiteral("Launching clang++..."));

    // Set up compiler process
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

    // Add OpenCV library path
    QString libDir = QString::fromLocal8Bit(OPENCV_LIB_DIR);
    arguments << QStringLiteral("-L") + libDir;

    // OpenCV libs
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
    _compileButton->setText(QStringLiteral("Compile & Apply"));

    if (exitStatus == QProcess::CrashExit) {
        _logConsole->append(QStringLiteral("\n[Error] Compiler process crashed!"));
        unloadLibrary();
        return;
    }

    if (exitCode == 0) {
        _logConsole->append(QStringLiteral("\n[Success] Compilation succeeded!"));
        loadLibrary();
    } else {
        QString errStr = QString::fromLocal8Bit(_compiler->readAllStandardError());
        _logConsole->append(QStringLiteral("\n[Failed] Compilation failed with code ") + QString::number(exitCode));
        _logConsole->append(errStr);
        unloadLibrary();
    }
#endif
}

void QProcessingWidget::loadLibrary() {
#ifdef HAS_OPENCV
    unloadLibrary();

    QString dylibPath = getScratchDir() + QStringLiteral("/libdynamic_filter.dylib");
    _logConsole->append(QStringLiteral("Loading library: ") + dylibPath);

    _library = new QLibrary(dylibPath, this);
    if (_library->load()) {
        _filterFunc = reinterpret_cast<ProcessFunc>(_library->resolve("processImage"));
        if (_filterFunc) {
            _logConsole->append(QStringLiteral("[OK] Dynamic filter active!"));
            emit filterReady(_filterFunc);
            // Trigger first state
            emit parametersChanged(static_cast<double>(_slider1->value()), static_cast<double>(_slider2->value()));
        } else {
            _logConsole->append(QStringLiteral("[Error] Symbol 'processImage' not found in library."));
            unloadLibrary();
        }
    } else {
        _logConsole->append(QStringLiteral("[Error] Failed to load library: ") + _library->errorString());
        unloadLibrary();
    }
#endif
}

void QProcessingWidget::unloadLibrary() {
#ifdef HAS_OPENCV
    _filterFunc = nullptr;
    emit filterReady(nullptr);

    if (_library) {
        if (_library->isLoaded()) {
            _library->unload();
        }
        delete _library;
        _library = nullptr;
    }
#endif
}
