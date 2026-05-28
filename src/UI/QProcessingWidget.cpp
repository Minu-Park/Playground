#include "UI/QProcessingWidget.h"
#include "Utility/ParameterParser.h"
#include "Controller/AbstractImagingController.h"
#include "Pipeline/DynamicProcessingCompiler.h"
#include "Pipeline/DynamicLibraryLoader.h"
#include "Pipeline/ProcessingPipeline.h"
#include "UI/RuntimePathsDialog.h"
#include <mutex>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QResizeEvent>
#include <QPushButton>
#include <QToolButton>
#include <QFrame>
#include <QGroupBox>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QPalette>
#include <QFont>
#include <QDebug>
#include <QCompleter>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QKeyEvent>
#include <QDoubleSpinBox>
#include <QStyle>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QStringListModel>
#include <QStandardPaths>
#include <QUuid>



class CppHighlighter : public QSyntaxHighlighter {
public:
    explicit CppHighlighter(QTextDocument* parent = nullptr) : QSyntaxHighlighter(parent) {
        HighlightingRule rule;

        // C++ Keywords (Navy Blue)
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor(QStringLiteral("#00457C")));
        keywordFormat.setFontWeight(QFont::Bold);
        QStringList keywordPatterns;
        keywordPatterns << QStringLiteral("\\bchar\\b") << QStringLiteral("\\bclass\\b") << QStringLiteral("\\bconst\\b")
                        << QStringLiteral("\\bdouble\\b") << QStringLiteral("\\belse\\b") << QStringLiteral("\\bextern\\b")
                        << QStringLiteral("\\bfloat\\b") << QStringLiteral("\\bfor\\b") << QStringLiteral("\\bif\\b")
                        << QStringLiteral("\\bint\\b") << QStringLiteral("\\breturn\\b") << QStringLiteral("\\bstruct\\b")
                        << QStringLiteral("\\bvoid\\b") << QStringLiteral("\\bwhile\\b") << QStringLiteral("\\bunsigned\\b")
                        << QStringLiteral("\\bstd\\b");
        for (const QString& pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            _rules.push_back(rule);
        }

        // OpenCV Keywords (Teal Green)
        QTextCharFormat cvFormat;
        cvFormat.setForeground(QColor(QStringLiteral("#007A87")));
        cvFormat.setFontWeight(QFont::Bold);
        rule.pattern = QRegularExpression(QStringLiteral("\\bcv::\\w+\\b"));
        rule.format = cvFormat;
        _rules.push_back(rule);

        // String Literals (Warm Amber)
        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor(QStringLiteral("#B25900")));
        rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
        rule.format = stringFormat;
        _rules.push_back(rule);

        // Numbers (Crimson)
        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor(QStringLiteral("#B02A37")));
        rule.pattern = QRegularExpression(QStringLiteral("\\b\\d+(\\.\\d+)?\\b"));
        rule.format = numberFormat;
        _rules.push_back(rule);

        // Single Line Comments (Muted Slate Grey)
        QTextCharFormat commentFormat;
        commentFormat.setForeground(QColor(QStringLiteral("#64748B")));
        rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
        rule.format = commentFormat;
        _rules.push_back(rule);
    }

protected:
    void highlightBlock(const QString& text) override {
        for (const HighlightingRule& rule : _rules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }

        // Multi-line comments /* */ (Muted Slate Grey)
        setCurrentBlockState(0);
        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf(QStringLiteral("/*"));
        }

        QTextCharFormat multiLineCommentFormat;
        multiLineCommentFormat.setForeground(QColor(QStringLiteral("#64748B")));

        while (startIndex >= 0) {
            QRegularExpressionMatch match = QRegularExpression(QStringLiteral("\\*/")).match(text, startIndex);
            int endIndex = match.capturedStart();
            int commentLength = 0;
            if (endIndex < 0) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + match.capturedLength();
            }
            setFormat(startIndex, commentLength, multiLineCommentFormat);
            startIndex = text.indexOf(QStringLiteral("/*"), startIndex + commentLength);
        }
    }

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    std::vector<HighlightingRule> _rules;
};

class QCodeEditor : public QTextEdit {
public:
    explicit QCodeEditor(QWidget* parent = nullptr) : QTextEdit(parent) {
        QStringList words;
        words << QStringLiteral("cv::Mat")
              << QStringLiteral("cv::Size")
              << QStringLiteral("cv::Point")
              << QStringLiteral("cv::Scalar")
              << QStringLiteral("cv::Rect")
              << QStringLiteral("cv::cvtColor")
              << QStringLiteral("cv::GaussianBlur")
              << QStringLiteral("cv::Canny")
              << QStringLiteral("cv::threshold")
              << QStringLiteral("cv::Sobel")
              << QStringLiteral("cv::Laplacian")
              << QStringLiteral("cv::bitwise_not")
              << QStringLiteral("cv::bitwise_and")
              << QStringLiteral("cv::bitwise_or")
              << QStringLiteral("cv::bitwise_xor")
              << QStringLiteral("cv::COLOR_BGR2GRAY")
              << QStringLiteral("cv::COLOR_BGRA2GRAY")
              << QStringLiteral("cv::COLOR_GRAY2BGR")
              << QStringLiteral("cv::COLOR_GRAY2BGRA")
              << QStringLiteral("cv::THRESH_BINARY")
              << QStringLiteral("cv::THRESH_BINARY_INV")
              << QStringLiteral("img")
              << QStringLiteral("param1")
              << QStringLiteral("param2")
              << QStringLiteral("param3")
              << QStringLiteral("param4")
              << QStringLiteral("width")
              << QStringLiteral("height")
              << QStringLiteral("channels");

        words.removeDuplicates();
        words.sort();

        const QString incDir = OpenCvBuildEnvironment::fromBuildDefaults().includeDir;
        QStringList opencvWords;

        auto parseHeader = [&](const QString& filePath) {
            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return;
            }

            QRegularExpression funcRegex(QStringLiteral("CV_EXPORTS_W\\s+[\\w:&*<>]+\\s+(\\w+)\\s*\\("));
            QRegularExpression classRegex(QStringLiteral("class\\s+CV_EXPORTS_W?\\s+(\\w+)"));

            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();

                // Match functions
                QRegularExpressionMatch match = funcRegex.match(line);
                if (match.hasMatch()) {
                    QString name = match.captured(1);
                    QString fullSymbol = QStringLiteral("cv::") + name;
                    if (!name.isEmpty() && !opencvWords.contains(fullSymbol) && !words.contains(fullSymbol)) {
                        opencvWords << fullSymbol;
                    }
                    continue;
                }

                // Match classes
                match = classRegex.match(line);
                if (match.hasMatch()) {
                    QString name = match.captured(1);
                    QString fullSymbol = QStringLiteral("cv::") + name;
                    if (!name.isEmpty() && !opencvWords.contains(fullSymbol) && !words.contains(fullSymbol)) {
                        opencvWords << fullSymbol;
                    }
                }
            }
        };

        if (!incDir.isEmpty()) {
            parseHeader(incDir + QStringLiteral("/opencv2/core.hpp"));
            parseHeader(incDir + QStringLiteral("/opencv2/imgproc.hpp"));
        }

        if (!opencvWords.isEmpty()) {
            words.append(opencvWords);
            words.removeDuplicates();
            words.sort();
        }
        _defaultWords = words;

        // Initialize cv::Mat member list
        _matMembers << QStringLiteral("cols")
                    << QStringLiteral("rows")
                    << QStringLiteral("channels()")
                    << QStringLiteral("empty()")
                    << QStringLiteral("depth()")
                    << QStringLiteral("type()")
                    << QStringLiteral("clone()")
                    << QStringLiteral("copyTo()")
                    << QStringLiteral("create()")
                    << QStringLiteral("data")
                    << QStringLiteral("step")
                    << QStringLiteral("reshape()")
                    << QStringLiteral("total()")
                    << QStringLiteral("size()")
                    << QStringLiteral("isContinuous()")
                    << QStringLiteral("elemSize()");

        _combinedWords = _defaultWords;
        _completer = new QCompleter(_combinedWords, this);
        _completer->setWidget(this);
        _completer->setCompletionMode(QCompleter::PopupCompletion);
        _completer->setCaseSensitivity(Qt::CaseInsensitive);

        // Template connect syntax works without Q_OBJECT because QTextEdit inherits QObject
        connect(_completer, QOverload<const QString&>::of(&QCompleter::activated),
                this, &QCodeEditor::insertCompletion);

        _highlighter = new CppHighlighter(document());
        connect(this, &QTextEdit::textChanged, this, &QCodeEditor::updateAutocompleteWords);
    }

    void setCompleter(QCompleter* completer) {
        if (_completer) {
            disconnect(_completer, nullptr, this, nullptr);
        }
        _completer = completer;
        if (_completer) {
            _completer->setWidget(this);
        }
    }

    QCompleter* completer() const { return _completer; }

protected:
    void keyPressEvent(QKeyEvent* e) override {
        if (_completer && _completer->popup()->isVisible()) {
            switch (e->key()) {
            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_Escape:
            case Qt::Key_Tab:
            case Qt::Key_Backtab:
                e->ignore();
                return;
            default:
                break;
            }
        }

        // Forward the key press to QTextEdit first so that the character is inserted
        QTextEdit::keyPressEvent(e);

        if (!_completer) return;

        // Get the current line text up to the cursor
        QTextCursor tc = textCursor();
        QTextCursor tcLine = tc;
        tcLine.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        QString lineText = tcLine.selectedText();

        // Check if cursor is after a cv::Mat variable followed by a dot
        QString pattern = QStringLiteral("(?:") + _matVariables.join(QStringLiteral("|")) + QStringLiteral(")\\.(\\w*)$");
        QRegularExpression matMemberRegex(pattern);
        QRegularExpressionMatch match = matMemberRegex.match(lineText);

        if (match.hasMatch()) {
            QString prefix = match.captured(1);

            // Swap completer model to Mat members
            QStringListModel* model = qobject_cast<QStringListModel*>(_completer->model());
            if (model) {
                if (model->stringList() != _matMembers) {
                    model->setStringList(_matMembers);
                }
            }
            _completer->setCompletionPrefix(prefix);

            QRect cr = cursorRect();
            cr.setWidth(_completer->popup()->sizeHintForColumn(0) + _completer->popup()->verticalScrollBar()->sizeHint().width());
            _completer->complete(cr); // Trigger popup
        } else {
            // Restore default OpenCV word list
            QStringListModel* model = qobject_cast<QStringListModel*>(_completer->model());
            if (model) {
                if (model->stringList() != _combinedWords) {
                    model->setStringList(_combinedWords);
                }
            }

            // Normal autocomplete trigger (only if not a modifier key and prefix is long enough)
            const bool isShortcut = (e->modifiers().testFlag(Qt::ControlModifier) && e->key() == Qt::Key_Space);
            const bool ctrlOrShift = e->modifiers().testFlag(Qt::ControlModifier) || e->modifiers().testFlag(Qt::ShiftModifier);

            if (ctrlOrShift && e->text().isEmpty() && !isShortcut) {
                return;
            }

            static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-=");
            const bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
            QString completionPrefix = textUnderCursor();

            if (!isShortcut && (hasModifier || e->text().isEmpty() || completionPrefix.length() < 2 || eow.contains(e->text().right(1)))) {
                _completer->popup()->hide();
                return;
            }

            if (completionPrefix != _completer->completionPrefix()) {
                _completer->setCompletionPrefix(completionPrefix);
                _completer->popup()->setCurrentIndex(_completer->completionModel()->index(0, 0));
            }

            QRect cr = cursorRect();
            cr.setWidth(_completer->popup()->sizeHintForColumn(0) + _completer->popup()->verticalScrollBar()->sizeHint().width());
            _completer->complete(cr);
        }
    }

    void focusInEvent(QFocusEvent* e) override {
        if (_completer) {
            _completer->setWidget(this);
        }
        QTextEdit::focusInEvent(e);
    }

public:
    void setCustomWords(const QStringList& words) {
        _customWords = words;
        updateAutocompleteWords();
    }

private:
    void insertCompletion(const QString& completion) {
        if (_completer->widget() != this) {
            return;
        }
        QTextCursor tc = textCursor();
        int extra = completion.length() - _completer->completionPrefix().length();
        tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, _completer->completionPrefix().length());
        tc.removeSelectedText();
        tc.insertText(completion);
        setTextCursor(tc);
    }

    QString textUnderCursor() const {
        QTextCursor tc = textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        QString word = tc.selectedText();

        QTextCursor tcBefore = textCursor();
        tcBefore.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, word.length());
        tcBefore.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 4);
        if (tcBefore.selectedText() == QStringLiteral("cv::")) {
            word = QStringLiteral("cv::") + word;
        }
        return word;
    }

    void updateAutocompleteWords() {
        QString code = toPlainText();
        QStringList customWords = _customWords; // Keep the parameter custom words set externally

        // Extract cv::Mat variables
        QStringList matVars;
        QRegularExpression matRegex(QStringLiteral("\\bcv::Mat\\s+([^;]+);"));
        QRegularExpressionMatchIterator matIt = matRegex.globalMatch(code);
        while (matIt.hasNext()) {
            QRegularExpressionMatch match = matIt.next();
            QString decls = match.captured(1);
            QStringList parts = decls.split(QStringLiteral(","));
            for (const QString& part : parts) {
                QString var = part.split(QStringLiteral("=")).first().trimmed();
                QRegularExpression idRegex(QStringLiteral("^[a-zA-Z_][a-zA-Z0-9_]*$"));
                if (idRegex.match(var).hasMatch()) {
                    customWords << var;
                    matVars << var;
                }
            }
        }

        // Extract standard data types
        QRegularExpression typeRegex(QStringLiteral("\\b(int|double|float|bool|auto|char|size_t)\\s+([^;]+);"));
        QRegularExpressionMatchIterator typeIt = typeRegex.globalMatch(code);
        while (typeIt.hasNext()) {
            QRegularExpressionMatch match = typeIt.next();
            QString decls = match.captured(2);
            QStringList parts = decls.split(QStringLiteral(","));
            for (const QString& part : parts) {
                QString var = part.split(QStringLiteral("=")).first().trimmed();
                QRegularExpression idRegex(QStringLiteral("^[a-zA-Z_][a-zA-Z0-9_]*$"));
                if (idRegex.match(var).hasMatch()) {
                    customWords << var;
                }
            }
        }

        _matVariables = matVars;
        // Add default image/mat variables
        _matVariables << QStringLiteral("img") << QStringLiteral("gray") << QStringLiteral("blurred") << QStringLiteral("edges") << QStringLiteral("mat");
        _matVariables.removeDuplicates();

        _combinedWords = _defaultWords + customWords;
        _combinedWords.removeDuplicates();
        _combinedWords.sort();

        if (_completer) {
            QStringListModel* model = qobject_cast<QStringListModel*>(_completer->model());
            if (model) {
                model->setStringList(_combinedWords);
            }
        }
    }

    QCompleter* _completer = nullptr;
    CppHighlighter* _highlighter = nullptr;
    QStringList _defaultWords;
    QStringList _customWords;
    QStringList _combinedWords;
    QStringList _matMembers;
    QStringList _matVariables;
};

using ProcessImageFunc = void (*)(unsigned char*, int, int, int, int, const double*, int);

class DynamicWrapperNode : public ProcessingNode {
public:
    DynamicWrapperNode() {
        _specs = {
            { QStringLiteral("Parameter 1"), 1.0, 100.0, 5.0, 0 },
            { QStringLiteral("Parameter 2"), 1.0, 250.0, 50.0, 0 }
        };
        _paramValues = { 5.0, 50.0 };
    }

    bool isLoaded() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _func != nullptr;
    }

    QString name() const override { return QStringLiteral("OpenCV Filter Script"); }

    std::vector<ParameterSpec> parameterSpecs() const override {
        std::lock_guard<std::mutex> lock(_mutex);
        return _specs;
    }

    double getParameter(int index) const override {
        std::lock_guard<std::mutex> lock(_mutex);
        if (index >= 0 && index < static_cast<int>(_paramValues.size())) {
            return _paramValues[index];
        }
        return 0.0;
    }

    void setParameter(int index, double value) override {
        std::lock_guard<std::mutex> lock(_mutex);
        if (index >= 0 && index < static_cast<int>(_paramValues.size())) {
            _paramValues[index] = value;
        }
    }

    void updateLibraryAndSpecs(std::shared_ptr<DynamicLibraryLoader> loader, ProcessImageFunc func, const std::vector<ParameterSpec>& newSpecs) {
        std::lock_guard<std::mutex> lock(_mutex);
        _loader = std::move(loader);
        _func = func;
        _specs = newSpecs;

        std::vector<double> newValues(newSpecs.size(), 0.0);
        for (size_t i = 0; i < newSpecs.size(); ++i) {
            if (i < _paramValues.size()) {
                newValues[i] = _paramValues[i];
            } else {
                newValues[i] = newSpecs[i].defaultValue;
            }
        }
        _paramValues = newValues;
    }

    void process(ProcessingFrame& frame) override {
        ProcessImageFunc activeFunc = nullptr;
        std::shared_ptr<DynamicLibraryLoader> activeLoader;
        std::vector<double> values;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            activeFunc = _func;
            activeLoader = _loader;
            values = _paramValues;
        }

        if (!activeFunc) return; // Pass-through if not compiled

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

            activeFunc(img.bits(), img.width(), img.height(), channels, img.bytesPerLine(), values.data(), static_cast<int>(values.size()));
        }
    }

private:
    std::shared_ptr<DynamicLibraryLoader> _loader;
    ProcessImageFunc _func = nullptr;
    std::vector<ParameterSpec> _specs;
    std::vector<double> _paramValues;
    mutable std::mutex _mutex;
};

QProcessingWidget::QProcessingWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(380, 480);
    QString scratchRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (scratchRoot.isEmpty()) {
        scratchRoot = QDir::tempPath();
    }
    const QString sessionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    _scratchDir = QDir(scratchRoot).filePath(QStringLiteral("Playground/processing/%1").arg(sessionId));
    QString autosaveRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (autosaveRoot.isEmpty()) {
        autosaveRoot = QDir::tempPath();
    }
    _autosaveDir = QDir(autosaveRoot).filePath(QStringLiteral("processing/autosave"));
    _scriptNode = std::make_shared<DynamicWrapperNode>();
    _scriptNode->setEnabled(true);
    initUI();
    refreshParameterUI();
}

QProcessingWidget::~QProcessingWidget() {
    if (_compiler) {
        _compiler->kill();
        _compiler->waitForFinished();
    }
}

QSize QProcessingWidget::sizeHint() const {
    return QSize(450, 600); // Generous default width and height for code editing and sliders
}

QSize QProcessingWidget::minimumSizeHint() const {
    return QSize(380, 480); // Ensure minimum usable space for layout
}

void QProcessingWidget::setController(AbstractImagingController* controller) {
    _controller = controller;
    if (_controller) {
        updatePipelineInController();
    }
}

void QProcessingWidget::initUI() {
    // Clean up any left-over temporary dynamic library files safely during initialization
    QString scratchDir = getScratchDir();
    QDir dir(scratchDir);
    if (dir.exists()) {
        QStringList filters;
        filters << QStringLiteral("*.dylib") << QStringLiteral("*.so") << QStringLiteral("*.dll");
        QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);
        for (const QFileInfo& fileInfo : fileList) {
            QFile::remove(fileInfo.absoluteFilePath());
        }
    }

    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(12, 12, 12, 12);
    _mainLayout->setSpacing(8);

    // Editor Title & Editor
    QHBoxLayout* editorHeaderLayout = new QHBoxLayout();
    editorHeaderLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* editorTitle = new QLabel(QStringLiteral("OpenCV Scripting (C++ 20)"), this);
    editorTitle->setObjectName(QStringLiteral("ProcessingEditorTitle"));
    editorHeaderLayout->addWidget(editorTitle);
    editorHeaderLayout->addStretch(1);

    _enableToggleBtn = new QToolButton(this);
    _enableToggleBtn->setObjectName(QStringLiteral("ProcessingEnableToggle"));
    _enableToggleBtn->setCheckable(true);
    _enableToggleBtn->setChecked(_scriptNode->isEnabled());

    auto updateToggleStyle = [this](bool checked) {
        _enableToggleBtn->setProperty("active", checked);
        if (checked) {
            _enableToggleBtn->setText(QStringLiteral("Active"));
        } else {
            _enableToggleBtn->setText(QStringLiteral("Bypass"));
        }
        _enableToggleBtn->style()->unpolish(_enableToggleBtn);
        _enableToggleBtn->style()->polish(_enableToggleBtn);
    };

    updateToggleStyle(_enableToggleBtn->isChecked());

    connect(_enableToggleBtn, &QToolButton::toggled, this, [this, updateToggleStyle](bool checked) {
        if (_scriptNode) {
            _scriptNode->setEnabled(checked);
        }
        updateToggleStyle(checked);
    });

    _runtimePathsButton = new QToolButton(this);
    _runtimePathsButton->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-setting-48.png")));
    _runtimePathsButton->setToolTip(QStringLiteral("OpenCV runtime paths"));
    connect(_runtimePathsButton, &QToolButton::clicked, this, [this]() {
        RuntimePathsDialog dialog(this);
        dialog.exec();
    });
    editorHeaderLayout->addWidget(_runtimePathsButton);

    _compileButton = new QToolButton(this);
    _compileButton->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-run-command-48.png")));
    _compileButton->setToolTip(QStringLiteral("Compile & Apply Hot-Swap"));
    connect(_compileButton, &QToolButton::clicked, this, &QProcessingWidget::handleCompile);
    editorHeaderLayout->addWidget(_compileButton);

    editorHeaderLayout->addWidget(_enableToggleBtn);
    _mainLayout->addLayout(editorHeaderLayout);

    QFont monoFont(QStringLiteral("Courier New"), 12);
    monoFont.setStyleHint(QFont::Monospace);

    _codeEditor = new QCodeEditor(this);
    _codeEditor->setObjectName(QStringLiteral("ProcessingCodeEditor"));
    _codeEditor->setFont(monoFont);
    _codeEditor->setMinimumHeight(150);

    // Template code (now without parameter comments)
    QString templateCode =
        "// Available variables:\n"
        "//   cv::Mat img       - The raw image to process (BGRA, BGR, or Grayscale)\n"
        "//   int width, height - Dimensions of the image\n"
        "//   int channels      - Number of channels (4 for BGRA, 3 for BGR, 1 for Mono)\n"
        "//\n"
        "// Write your OpenCV script below (modifies 'img' in-place):\n\n"
        "cv::Mat gray, blurred, edges;\n"
        "if (channels == 4) {\n"
        "    cv::cvtColor(img, gray, cv::COLOR_BGRA2GRAY);\n"
        "} else if (channels == 3) {\n"
        "    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);\n"
        "} else {\n"
        "    gray = img;\n"
        "}\n\n"
        "// Apply Gaussian blur (kSize is dynamically available)\n"
        "int kernelSize = std::max(1, (int(kSize) % 2 == 0) ? int(kSize) + 1 : int(kSize));\n"
        "cv::GaussianBlur(gray, blurred, cv::Size(kernelSize, kernelSize), 0);\n\n"
        "// Apply Canny edge detection (lowThresh is dynamically available)\n"
        "cv::Canny(blurred, edges, lowThresh, lowThresh * 3);\n\n"
        "// Convert edges result back to target channels in 'img'\n"
        "if (channels == 4) {\n"
        "    cv::cvtColor(edges, img, cv::COLOR_GRAY2BGRA);\n"
        "} else if (channels == 3) {\n"
        "    cv::cvtColor(edges, img, cv::COLOR_GRAY2BGR);\n"
        "} else {\n"
        "    edges.copyTo(img);\n"
        "}\n";

    _mainLayout->addWidget(_codeEditor, 1);

    // 3. Filter Parameters Container (No Group Box)
    auto* paramContainer = new QWidget(this);
    auto* paramContainerLayout = new QVBoxLayout(paramContainer);
    paramContainerLayout->setContentsMargins(0, 0, 0, 0);
    paramContainerLayout->setSpacing(6);

    // 3a. Parameter Definitions Title & Editor
    auto* paramEditorTitle = new QLabel(QStringLiteral("Parameter Definitions:"), paramContainer);
    paramEditorTitle->setObjectName(QStringLiteral("ProcessingParamEditorTitle"));
    paramContainerLayout->addWidget(paramEditorTitle);

    _paramEditor = new QTextEdit(paramContainer);
    _paramEditor->setObjectName(QStringLiteral("ProcessingParamEditor"));
    _paramEditor->setMinimumHeight(60);
    _paramEditor->setMaximumHeight(90);
    _paramEditor->setPlaceholderText(QStringLiteral("@param: varName, name=\"Display Name\", min=0, max=100, default=50\n@param: ..."));

    // Default template parameters
    QString templateParams =
        "@param: kSize, name=\"Kernel Size\", min=1, max=21, default=5\n"
        "@param: lowThresh, name=\"Canny Low\", min=10, max=150, default=50\n";

    paramContainerLayout->addWidget(_paramEditor);

    // 3b. Dynamic Sliders Layout
    _paramLayout = new QVBoxLayout();
    _paramLayout->setContentsMargins(0, 4, 0, 0);
    _paramLayout->setSpacing(8);
    paramContainerLayout->addLayout(_paramLayout);

    _mainLayout->addWidget(paramContainer);

    // Connect paramEditor textChanged to parse parameters dynamically and update autocompleter
    connect(_paramEditor, &QTextEdit::textChanged, this, [this]() {
        auto parsed = ParameterParser::parseCode(_paramEditor->toPlainText());
        QStringList customWords;
        for (const auto& p : parsed) {
            if (!p.varName.isEmpty()) {
                customWords << p.varName;
            }
        }
        _codeEditor->setCustomWords(customWords);
        refreshParameterUI();
    });

    // Try loading saved temp code or set template code
    QString tempPath = getAutosaveDir() + QStringLiteral("/dynamic_filter_temp.cpp");
    QFile tempFile(tempPath);
    if (tempFile.exists() && tempFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&tempFile);
        _codeEditor->setPlainText(in.readAll());
        tempFile.close();
    } else {
        _codeEditor->setPlainText(templateCode);
    }

    // Try loading saved temp params or set template params
    QString tempParamsPath = getAutosaveDir() + QStringLiteral("/dynamic_filter_temp_params.txt");
    QFile tempParamsFile(tempParamsPath);
    if (tempParamsFile.exists() && tempParamsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&tempParamsFile);
        _paramEditor->setPlainText(in.readAll());
        tempParamsFile.close();
    } else {
        _paramEditor->setPlainText(templateParams);
    }

    // Connect textChanged slots for auto saving
    connect(_codeEditor, &QTextEdit::textChanged, this, [this]() {
        QString autosaveDir = getAutosaveDir();
        QDir().mkpath(autosaveDir);
        QFile file(autosaveDir + QStringLiteral("/dynamic_filter_temp.cpp"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << _codeEditor->toPlainText();
            file.close();
        }
    });

    connect(_paramEditor, &QTextEdit::textChanged, this, [this]() {
        QString autosaveDir = getAutosaveDir();
        QDir().mkpath(autosaveDir);
        QFile file(autosaveDir + QStringLiteral("/dynamic_filter_temp_params.txt"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << _paramEditor->toPlainText();
            file.close();
        }
    });

    // Trigger parsing once initially
    emit _paramEditor->textChanged();
}

void QProcessingWidget::refreshParameterUI() {
    // 1. Clear existing dynamic controls
    for (auto& ctrl : _dynamicControls) {
        if (ctrl.controlWidget) {
            ctrl.controlWidget->deleteLater();
        }
        if (ctrl.nameLabel) {
            ctrl.nameLabel->deleteLater();
        }
        if (ctrl.valueLabel) {
            ctrl.valueLabel->deleteLater();
        }
    }
    _dynamicControls.clear();

    // Recursively clear layout items to prevent memory leaks and overlapping items
    if (_paramLayout) {
        QLayoutItem* child;
        while ((child = _paramLayout->takeAt(0)) != nullptr) {
            if (child->layout()) {
                QLayout* subLayout = child->layout();
                QLayoutItem* subChild;
                while ((subChild = subLayout->takeAt(0)) != nullptr) {
                    if (subChild->widget()) {
                        subChild->widget()->deleteLater();
                    }
                    delete subChild;
                }
            } else if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }
    }

    if (!_scriptNode) return;

    if (!_scriptNode->isLoaded()) {
        auto* waitLabel = new QLabel(QStringLiteral("Compile and apply the filter to configure parameters."), this);
        waitLabel->setObjectName(QStringLiteral("ProcessingParamHintLabel"));
        _paramLayout->addWidget(waitLabel);

        ParameterControl ctrl;
        ctrl.index = -1;
        ctrl.nameLabel = waitLabel;
        _dynamicControls.push_back(ctrl);
        return;
    }

    // 2. Fetch parameters specs from the node
    auto specs = _scriptNode->parameterSpecs();
    if (specs.empty()) {
        auto* noParamLabel = new QLabel(QStringLiteral("No adjustable parameters for this script."), this);
        noParamLabel->setObjectName(QStringLiteral("ProcessingParamEmptyLabel"));
        _paramLayout->addWidget(noParamLabel);

        ParameterControl ctrl;
        ctrl.index = -1;
        ctrl.nameLabel = noParamLabel;
        _dynamicControls.push_back(ctrl);
        return;
    }

    // 3. Rebuild controls
    for (size_t i = 0; i < specs.size(); ++i) {
        const auto& spec = specs[i];
        double currentVal = _scriptNode->getParameter(static_cast<int>(i));

        auto* headerLayout = new QHBoxLayout();
        auto* nameLbl = new QLabel(spec.name, this);
        nameLbl->setObjectName(QStringLiteral("ProcessingParamNameLabel"));
        auto* valLbl = new QLabel(QString::number(currentVal, 'f', spec.decimals), this);
        valLbl->setObjectName(QStringLiteral("ProcessingParamValueLabel"));

        headerLayout->addWidget(nameLbl);
        headerLayout->addWidget(valLbl, 0, Qt::AlignRight);
        _paramLayout->addLayout(headerLayout);

        QWidget* controlWidget = nullptr;
        if (spec.decimals == 0) {
            auto* slider = new QSlider(Qt::Horizontal, this);
            slider->setObjectName(QStringLiteral("ProcessingParamSlider"));
            slider->setRange(static_cast<int>(spec.minValue), static_cast<int>(spec.maxValue));
            slider->setValue(static_cast<int>(currentVal));

            connect(slider, &QSlider::valueChanged, this, &QProcessingWidget::handleDynamicSliderChanged);
            _paramLayout->addWidget(slider);
            controlWidget = slider;
        } else {
            auto* spin = new QDoubleSpinBox(this);
            spin->setRange(spec.minValue, spec.maxValue);
            spin->setValue(currentVal);
            spin->setDecimals(spec.decimals);
            spin->setSingleStep(0.1);

            connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &QProcessingWidget::handleDynamicSpinBoxChanged);
            _paramLayout->addWidget(spin);
            controlWidget = spin;
        }

        ParameterControl ctrl;
        ctrl.index = static_cast<int>(i);
        ctrl.nameLabel = nameLbl;
        ctrl.valueLabel = valLbl;
        ctrl.controlWidget = controlWidget;
        _dynamicControls.push_back(ctrl);
    }
}

void QProcessingWidget::handleDynamicSliderChanged() {
    auto* slider = qobject_cast<QSlider*>(sender());
    if (!slider) return;
    if (!_scriptNode) return;

    for (auto& ctrl : _dynamicControls) {
        if (ctrl.controlWidget == slider) {
            double val = static_cast<double>(slider->value());
            if (ctrl.valueLabel) {
                ctrl.valueLabel->setText(QString::number(slider->value()));
            }
            _scriptNode->setParameter(ctrl.index, val);
            break;
        }
    }
}

void QProcessingWidget::handleDynamicSpinBoxChanged(double value) {
    auto* spin = qobject_cast<QDoubleSpinBox*>(sender());
    if (!spin) return;
    if (!_scriptNode) return;

    for (auto& ctrl : _dynamicControls) {
        if (ctrl.controlWidget == spin) {
            if (ctrl.valueLabel) {
                ctrl.valueLabel->setText(QString::number(value, 'f', spin->decimals()));
            }
            _scriptNode->setParameter(ctrl.index, value);
            break;
        }
    }
}

void QProcessingWidget::updatePipelineInController() {
    if (!_controller) return;
    auto* pipe = _controller->pipeline();
    if (!pipe) return;

    pipe->clearNodes();
    if (_scriptNode) {
        pipe->addNode(_scriptNode);
    }
}

QString QProcessingWidget::getScratchDir() const {
    return _scratchDir;
}

QString QProcessingWidget::getAutosaveDir() const {
    return _autosaveDir;
}

void QProcessingWidget::handleCompile() {
    if (!_codeEditor || !_compileButton) return;

    _compileButton->setEnabled(false);
    _compileButton->setText(QStringLiteral("Compiling..."));
    qInfo() << "[Pipeline] Saving dynamic filter script...";

    DynamicProcessingCompileRequest request;
    request.scriptCode = _codeEditor->toPlainText();
    request.parameters = ParameterParser::parseCode(_paramEditor->toPlainText());
    request.scratchDir = getScratchDir();

    DynamicProcessingCompilePlan plan;
    QString error;
    const OpenCvBuildEnvironment environment = OpenCvBuildEnvironment::fromBuildDefaults();
    if (!DynamicProcessingCompiler::prepareOpenCvProcessImageBuild(environment, request, &plan, &error)) {
        qCritical() << "[Pipeline] [Error]" << error;
        _compileButton->setEnabled(true);
        _compileButton->setText(QStringLiteral("Compile & Apply Hot-Swap"));
        return;
    }

    qInfo() << "[Pipeline] Launching compiler:" << plan.compilerPath;

    if (_compiler) {
        _compiler->kill();
        _compiler->deleteLater();
    }

    _compiler = new QProcess(this);
    _compiler->setWorkingDirectory(plan.workingDirectory);

    // Set library path property to retrieve it asynchronously upon completion
    _compiler->setProperty("libraryPath", plan.outputPath);

    qInfo() << "[Pipeline] Command:" << plan.compilerPath << plan.arguments.join(QStringLiteral(" "));

    connect(_compiler, &QProcess::finished, this, &QProcessingWidget::handleCompilerFinished);
    _compiler->start(plan.compilerPath, plan.arguments);
}

void QProcessingWidget::handleCompilerFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    _compileButton->setEnabled(true);
    _compileButton->setText(QStringLiteral("Compile & Apply Hot-Swap"));

    QProcess* proc = qobject_cast<QProcess*>(sender());
    if (!proc) return;
    QString libraryPath = proc->property("libraryPath").toString();

    if (exitStatus == QProcess::CrashExit) {
        qCritical() << "[Pipeline] [Error] Compiler process crashed!";
        return;
    }

    if (exitCode == 0) {
        qInfo() << "[Pipeline] [Success] Compilation succeeded!";
        loadDynamicNode(libraryPath);
    } else {
        QString errStr = QString::fromLocal8Bit(proc->readAllStandardError());
        qCritical() << "[Pipeline] [Failed] Compilation failed with code" << exitCode;
        qWarning() << "[Pipeline] Compiler output:\n" << errStr;
    }
}

void QProcessingWidget::loadDynamicNode(const QString& dylibPath) {
    qInfo() << "[Pipeline] Loading dylib:" << dylibPath;

    // 1. Load the dynamic library loader
    auto loader = DynamicLibraryLoader::load(dylibPath);
    if (!loader) {
        qCritical() << "[Pipeline] [Error] Failed to load library.";
        return;
    }

    // 2. Resolve 'process_image'
    using ProcessImageFunc = void (*)(unsigned char*, int, int, int, int, const double*, int);
    auto func = reinterpret_cast<ProcessImageFunc>(loader->resolve("process_image"));
    if (!func) {
        qCritical() << "[Pipeline] [Error] Symbol 'process_image' not resolved.";
        return;
    }

    // Keep the installed function's library mapped through the persistent node.
    if (_scriptNode) {
        std::vector<ParameterSpec> newSpecs;
        if (_paramEditor) {
            auto parsed = ParameterParser::parseCode(_paramEditor->toPlainText());
            for (const auto& p : parsed) {
                newSpecs.push_back({ p.displayName, p.minValue, p.maxValue, p.defaultValue, p.decimals });
            }
        }
        _scriptNode->updateLibraryAndSpecs(loader, func, newSpecs);
    }

    qInfo() << "[Pipeline] [Success] OpenCV Filter Script active and injected.";

    refreshParameterUI();
}
