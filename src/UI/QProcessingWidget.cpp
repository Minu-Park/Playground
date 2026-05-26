#include "UI/QProcessingWidget.h"
#include "Controller/AbstractImagingController.h"
#include "Pipeline/ProcessingRegistry.h"
#include "Pipeline/DynamicLibraryLoader.h"
#include "Pipeline/ProcessingPipeline.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QBoxLayout>
#include <QResizeEvent>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QTabWidget>
#include <QGroupBox>
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
#include <QCompleter>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QKeyEvent>
#include <QDoubleSpinBox>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QStringListModel>



class CppHighlighter : public QSyntaxHighlighter {
public:
    explicit CppHighlighter(QTextDocument* parent = nullptr) : QSyntaxHighlighter(parent) {
        HighlightingRule rule;

        // C++ Keywords
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor(QStringLiteral("#569CD6")));
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

        // OpenCV Keywords (cv::Mat, cv::Canny, etc.)
        QTextCharFormat cvFormat;
        cvFormat.setForeground(QColor(QStringLiteral("#4EC9B0")));
        cvFormat.setFontWeight(QFont::Bold);
        rule.pattern = QRegularExpression(QStringLiteral("\\bcv::\\w+\\b"));
        rule.format = cvFormat;
        _rules.push_back(rule);

        // String Literals
        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor(QStringLiteral("#D69D85")));
        rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
        rule.format = stringFormat;
        _rules.push_back(rule);

        // Numbers
        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor(QStringLiteral("#B5CEA8")));
        rule.pattern = QRegularExpression(QStringLiteral("\\b\\d+(\\.\\d+)?\\b"));
        rule.format = numberFormat;
        _rules.push_back(rule);

        // Single Line Comments
        QTextCharFormat commentFormat;
        commentFormat.setForeground(QColor(QStringLiteral("#6A9955")));
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

        // Multi-line comments /* */
        setCurrentBlockState(0);
        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf(QStringLiteral("/*"));
        }

        QTextCharFormat multiLineCommentFormat;
        multiLineCommentFormat.setForeground(QColor(QStringLiteral("#6A9955")));

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

#ifdef HAS_OPENCV
        QString incDir = QString::fromLocal8Bit(OPENCV_INCLUDE_DIR);
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

        parseHeader(incDir + QStringLiteral("/opencv2/core.hpp"));
        parseHeader(incDir + QStringLiteral("/opencv2/imgproc.hpp"));

        if (!opencvWords.isEmpty()) {
            words.append(opencvWords);
            words.removeDuplicates();
            words.sort();
        }
#endif

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

        _completer = new QCompleter(_defaultWords, this);
        _completer->setWidget(this);
        _completer->setCompletionMode(QCompleter::PopupCompletion);
        _completer->setCaseSensitivity(Qt::CaseInsensitive);

        // Template connect syntax works without Q_OBJECT because QTextEdit inherits QObject
        connect(_completer, QOverload<const QString&>::of(&QCompleter::activated),
                this, &QCodeEditor::insertCompletion);

        _highlighter = new CppHighlighter(document());
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

        // Check if cursor is after a cv::Mat variable (e.g. img, gray, blurred, edges, mat) followed by a dot
        QRegularExpression matMemberRegex(QStringLiteral("(?:img|mat|gray|blurred|edges)\\.(\\w*)$"));
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
                if (model->stringList() != _defaultWords) {
                    model->setStringList(_defaultWords);
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

    QCompleter* _completer = nullptr;
    CppHighlighter* _highlighter = nullptr;
    QStringList _defaultWords;
    QStringList _matMembers;
};

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
    _mainLayout = new QVBoxLayout(this);
    _mainLayout->setContentsMargins(4, 4, 4, 4);

    _tabWidget = new QTabWidget(this);
    _mainLayout->addWidget(_tabWidget);

    // ==========================================
    // TAB 1: Pipeline & Parameters
    // ==========================================
    auto* pipelineTab = new QWidget(this);
    auto* pipelineLayout = new QVBoxLayout(pipelineTab);
    pipelineLayout->setContentsMargins(8, 8, 8, 8);
    pipelineLayout->setSpacing(8);

    auto* listsLayout = new QVBoxLayout();
    listsLayout->setSpacing(4);

    auto* availLabel = new QLabel(QStringLiteral("Available Filters:"), this);
    availLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #354657;"));
    _availableList = new QListWidget(this);
    _availableList->setMinimumHeight(100);

    auto* pipelineLabel = new QLabel(QStringLiteral("Active Pipeline (Check to enable):"), this);
    pipelineLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #354657;"));
    _pipelineList = new QListWidget(this);
    _pipelineList->setMinimumHeight(120);

    // Connect list widget change to update controller
    connect(_pipelineList, &QListWidget::itemChanged, this, &QProcessingWidget::handleNodeItemChanged);
    connect(_pipelineList, &QListWidget::itemSelectionChanged, this, &QProcessingWidget::handlePipelineSelectionChanged);

    // Compact toolbar for list operations (using icons)
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);
    btnLayout->setContentsMargins(0, 4, 0, 4);

    _addButton = new QToolButton(this);
    _addButton->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-add-image-48.png")));
    _addButton->setIconSize(QSize(16, 16));
    _addButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    _addButton->setText(QStringLiteral("Add"));
    _addButton->setToolTip(QStringLiteral("Add selected filter to active pipeline"));
    _addButton->setStyleSheet(QStringLiteral(
        "QToolButton {"
        "  font-weight: bold;"
        "  border: 1px solid #cfd9e4;"
        "  border-radius: 8px;"
        "  padding: 4px 10px;"
        "  background: #ffffff;"
        "}"
        "QToolButton:hover {"
        "  background-color: #f3f3f3;"
        "  border-color: #b7c7d8;"
        "}"
    ));

    _removeButton = new QToolButton(this);
    _removeButton->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-delete-48.png")));
    _removeButton->setIconSize(QSize(16, 16));
    _removeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    _removeButton->setText(QStringLiteral("Remove"));
    _removeButton->setToolTip(QStringLiteral("Remove selected filter from pipeline"));
    _removeButton->setStyleSheet(QStringLiteral(
        "QToolButton {"
        "  font-weight: bold;"
        "  border: 1px solid #cfd9e4;"
        "  border-radius: 8px;"
        "  padding: 4px 10px;"
        "  background: #ffffff;"
        "}"
        "QToolButton:hover {"
        "  background-color: #fef0f0;"
        "  border-color: #fecbcb;"
        "  color: #c62828;"
        "}"
    ));

    _moveUpButton = new QToolButton(this);
    _moveUpButton->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-chevron-up-48.png")));
    _moveUpButton->setIconSize(QSize(16, 16));
    _moveUpButton->setToolTip(QStringLiteral("Move selected filter up"));
    _moveUpButton->setStyleSheet(QStringLiteral(
        "QToolButton {"
        "  border: 1px solid #cfd9e4;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "  background: #ffffff;"
        "}"
        "QToolButton:hover {"
        "  background-color: #f3f3f3;"
        "  border-color: #b7c7d8;"
        "}"
    ));

    _moveDownButton = new QToolButton(this);
    _moveDownButton->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-chevron-down-48.png")));
    _moveDownButton->setIconSize(QSize(16, 16));
    _moveDownButton->setToolTip(QStringLiteral("Move selected filter down"));
    _moveDownButton->setStyleSheet(QStringLiteral(
        "QToolButton {"
        "  border: 1px solid #cfd9e4;"
        "  border-radius: 8px;"
        "  padding: 4px;"
        "  background: #ffffff;"
        "}"
        "QToolButton:hover {"
        "  background-color: #f3f3f3;"
        "  border-color: #b7c7d8;"
        "}"
    ));

    btnLayout->addWidget(_addButton);
    btnLayout->addWidget(_removeButton);
    btnLayout->addStretch(1);
    btnLayout->addWidget(_moveUpButton);
    btnLayout->addWidget(_moveDownButton);

    connect(_addButton, &QToolButton::clicked, this, &QProcessingWidget::handleAddNode);
    connect(_removeButton, &QToolButton::clicked, this, &QProcessingWidget::handleRemoveNode);
    connect(_moveUpButton, &QToolButton::clicked, this, &QProcessingWidget::handleMoveUp);
    connect(_moveDownButton, &QToolButton::clicked, this, &QProcessingWidget::handleMoveDown);

    listsLayout->addWidget(availLabel);
    listsLayout->addWidget(_availableList, 1);
    listsLayout->addLayout(btnLayout);
    listsLayout->addWidget(pipelineLabel);
    listsLayout->addWidget(_pipelineList, 2);

    pipelineLayout->addLayout(listsLayout, 3);

    // Styled Parameter Editor Group
    auto* paramGroup = new QGroupBox(QStringLiteral("Selected Node Parameters"), this);
    paramGroup->setStyleSheet(QStringLiteral(
        "QGroupBox {"
        "  font-weight: bold;"
        "  color: #354657;"
        "  border: 1px solid #d9e1ea;"
        "  border-radius: 10px;"
        "  margin-top: 12px;"
        "  padding: 10px;"
        "  background-color: #ffffff;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 12px;"
        "  padding: 0 4px;"
        "}"
    ));
    _paramLayout = new QVBoxLayout(paramGroup);
    _paramLayout->setContentsMargins(8, 14, 8, 8);
    _paramLayout->setSpacing(8);

    _selectedNodeLabel = new QLabel(QStringLiteral("Selected Node: None"), this);
    _selectedNodeLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px; color: #00457C;"));
    _paramLayout->addWidget(_selectedNodeLabel);


    pipelineLayout->addWidget(paramGroup, 2);
    _tabWidget->addTab(pipelineTab, QIcon(QStringLiteral(":/Resources/Icons/icons8-setting-48.png")), QStringLiteral("Pipeline & Control"));

    // ==========================================
    // TAB 2: Dynamic Compiler (OpenCV)
    // ==========================================
    auto* compilerTab = new QWidget(this);
    auto* compilerLayout = new QVBoxLayout(compilerTab);
    compilerLayout->setContentsMargins(8, 8, 8, 8);
    compilerLayout->setSpacing(8);

    QFont monoFont(QStringLiteral("Courier New"), 12);
    monoFont.setStyleHint(QFont::Monospace);

    auto* logTitle = new QLabel(QStringLiteral("Compilation & Load Console:"), this);
    logTitle->setStyleSheet(QStringLiteral("font-weight: 600; color: #354657;"));
    _logConsole = new QTextEdit(this);
    _logConsole->setReadOnly(true);
    _logConsole->setFont(monoFont);
    _logConsole->setMinimumHeight(100);
    _logConsole->setStyleSheet(QStringLiteral("QTextEdit { background-color: #121212; color: #00ff00; border: 1px solid #333333; border-radius: 8px; padding: 6px; }"));
    _logConsole->append(QStringLiteral("[System] Pipeline console ready."));

#ifndef HAS_OPENCV
    auto* noOpenCVLabel = new QLabel(
        QStringLiteral("OpenCV is not enabled. C++ live compilation is disabled.\n"
                       "Using built-in processing registry."), this);
    noOpenCVLabel->setStyleSheet(QStringLiteral("color: #cc6600; font-weight: bold; font-size: 12px; padding: 12px; border: 1px dashed #cc6600; border-radius: 8px;"));
    noOpenCVLabel->setAlignment(Qt::AlignCenter);
    compilerLayout->addWidget(noOpenCVLabel, 1);
    compilerLayout->addWidget(logTitle);
    compilerLayout->addWidget(_logConsole, 0);
#else
    auto* editorTitle = new QLabel(QStringLiteral("Dynamic C++ OpenCV Filter Code (C++20):"), this);
    editorTitle->setStyleSheet(QStringLiteral("font-weight: 600; color: #354657;"));
    _codeEditor = new QCodeEditor(this);
    _codeEditor->setFont(monoFont);
    _codeEditor->setMinimumHeight(150);
    _codeEditor->setStyleSheet(QStringLiteral("QTextEdit { background-color: #1e1e1e; color: #d4d4d4; border: 1px solid #3c3c3c; border-radius: 8px; padding: 6px; }"));

    // Prepopulate with script-style template code (boilerplate is hidden)
    QString templateCode =
        "// Available variables:\n"
        "//   cv::Mat img       - The raw image to process (BGRA, BGR, or Grayscale)\n"
        "//   double param1     - Value from slider/parameter 1\n"
        "//   double param2     - Value from slider/parameter 2\n"
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
        "// Apply Gaussian blur (param1 controls kernel size)\n"
        "int kernelSize = std::max(1, (int(param1) % 2 == 0) ? int(param1) + 1 : int(param1));\n"
        "cv::GaussianBlur(gray, blurred, cv::Size(kernelSize, kernelSize), 0);\n\n"
        "// Apply Canny edge detection (param2 controls threshold)\n"
        "cv::Canny(blurred, edges, param2, param2 * 3);\n\n"
        "// Convert edges result back to target channels in 'img'\n"
        "if (channels == 4) {\n"
        "    cv::cvtColor(edges, img, cv::COLOR_GRAY2BGRA);\n"
        "} else if (channels == 3) {\n"
        "    cv::cvtColor(edges, img, cv::COLOR_GRAY2BGR);\n"
        "} else {\n"
        "    edges.copyTo(img);\n"
        "}\n";
    _codeEditor->setPlainText(templateCode);

    _compileButton = new QPushButton(QStringLiteral("Compile & Apply Hot-Swap"), this);
    _compileButton->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-refresh-48.png")));
    _compileButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #00457C;"
        "  color: white;"
        "  font-weight: bold;"
        "  border-radius: 8px;"
        "  padding: 6px 12px;"
        "  min-height: 28px;"
        "  max-height: 28px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #005A9E;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #00335C;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #cccccc;"
        "  color: #666666;"
        "}"
    );

    connect(_compileButton, &QPushButton::clicked, this, &QProcessingWidget::handleCompile);

    compilerLayout->addWidget(editorTitle);
    compilerLayout->addWidget(_codeEditor, 3);
    compilerLayout->addWidget(_compileButton);
    compilerLayout->addWidget(logTitle);
    compilerLayout->addWidget(_logConsole, 1);
#endif

    _tabWidget->addTab(compilerTab, QIcon(QStringLiteral(":/Resources/Icons/icons8-edit-48.png")), QStringLiteral("C++ Scripting"));

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

    int index = _pipelineList->currentRow();
    if (index < 0 || index >= (int)_activeNodes.size()) {
        _selectedNodeLabel->setText(QStringLiteral("Selected Node: None"));
        return;
    }

    auto& node = _activeNodes[index];
    _selectedNodeLabel->setText(QStringLiteral("Selected Node: %1").arg(node->name()));

    // 2. Fetch parameters specs from the node
    auto specs = node->parameterSpecs();
    if (specs.empty()) {
        auto* noParamLabel = new QLabel(QStringLiteral("No adjustable parameters for this node."), this);
        noParamLabel->setStyleSheet(QStringLiteral("color: #777777; font-style: italic; padding: 4px;"));
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
        double currentVal = node->getParameter(static_cast<int>(i));

        auto* headerLayout = new QHBoxLayout();
        auto* nameLbl = new QLabel(spec.name, this);
        nameLbl->setStyleSheet(QStringLiteral("color: #4a5a6a; font-weight: 600;"));
        auto* valLbl = new QLabel(QString::number(currentVal, 'f', spec.decimals), this);
        valLbl->setStyleSheet(QStringLiteral("font-weight: bold; color: #00457C;"));

        headerLayout->addWidget(nameLbl);
        headerLayout->addWidget(valLbl, 0, Qt::AlignRight);
        _paramLayout->addLayout(headerLayout);

        QWidget* controlWidget = nullptr;
        if (spec.decimals == 0) {
            auto* slider = new QSlider(Qt::Horizontal, this);
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

    int index = _pipelineList->currentRow();
    if (index < 0 || index >= (int)_activeNodes.size()) return;
    auto& node = _activeNodes[index];

    for (auto& ctrl : _dynamicControls) {
        if (ctrl.controlWidget == slider) {
            double val = static_cast<double>(slider->value());
            if (ctrl.valueLabel) {
                ctrl.valueLabel->setText(QString::number(slider->value()));
            }
            node->setParameter(ctrl.index, val);
            break;
        }
    }
}

void QProcessingWidget::handleDynamicSpinBoxChanged(double value) {
    auto* spin = qobject_cast<QDoubleSpinBox*>(sender());
    if (!spin) return;

    int index = _pipelineList->currentRow();
    if (index < 0 || index >= (int)_activeNodes.size()) return;
    auto& node = _activeNodes[index];

    for (auto& ctrl : _dynamicControls) {
        if (ctrl.controlWidget == spin) {
            if (ctrl.valueLabel) {
                ctrl.valueLabel->setText(QString::number(value, 'f', spin->decimals()));
            }
            node->setParameter(ctrl.index, value);
            break;
        }
    }
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
    out << "#include <opencv2/opencv.hpp>\n"
        << "#include <algorithm>\n\n"
        << "extern \"C\" {\n"
        << "    void process_image(unsigned char* data, int width, int height, int channels, int step, double param1, double param2) {\n"
        << "        int type = (channels == 4) ? CV_8UC4 : ((channels == 3) ? CV_8UC3 : CV_8UC1);\n"
        << "        cv::Mat img(height, width, type, data, step);\n\n"
        << "        // USER SCRIPT START\n"
        << _codeEditor->toPlainText() << "\n"
        << "        // USER SCRIPT END\n"
        << "    }\n"
        << "}\n";
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

        std::vector<ParameterSpec> parameterSpecs() const override {
            return {
                { QStringLiteral("Parameter 1"), 1.0, 100.0, 5.0, 0 },
                { QStringLiteral("Parameter 2"), 1.0, 250.0, 50.0, 0 }
            };
        }

        double getParameter(int index) const override {
            if (index == 0) return _p1;
            else if (index == 1) return _p2;
            return 0.0;
        }

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

    // Set parameters from default values
    dynamicNode->setParameter(0, 5.0);
    dynamicNode->setParameter(1, 50.0);

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

void QProcessingWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
}

