#include "UI/RuntimePathsDialog.h"

#include "Pipeline/DynamicProcessingCompiler.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QIcon>
#include <QStyleOption>

namespace {

constexpr auto kCompilerPathKey = "RuntimePaths/OpenCV/CompilerPath";
constexpr auto kIncludeDirKey = "RuntimePaths/OpenCV/IncludeDir";
constexpr auto kLibraryDirKey = "RuntimePaths/OpenCV/LibraryDir";
constexpr auto kLibrariesKey = "RuntimePaths/OpenCV/Libraries";

class RuntimePathsTitleBar final : public QWidget
{
public:
    explicit RuntimePathsTitleBar(QDialog* parentDialog, const QString& title)
        : QWidget(parentDialog)
        , m_parentDialog(parentDialog)
    {
        setObjectName(QStringLiteral("RuntimePathsTitleBar"));
        setFixedHeight(28);

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 0, 8, 0);
        layout->setSpacing(6);

        auto* titleLabel = new QLabel(title, this);
        titleLabel->setObjectName(QStringLiteral("RuntimePathsTitleLabel"));
        titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        titleLabel->setStyleSheet(QStringLiteral("color: #16202b; font-weight: bold; font-size: 11px;"));
        layout->addWidget(titleLabel, 0, Qt::AlignVCenter);

        layout->addStretch();

        auto* closeButton = new QPushButton(this);
        closeButton->setObjectName(QStringLiteral("RuntimePathsCloseButton"));
        closeButton->setFocusPolicy(Qt::NoFocus);
        closeButton->setFixedSize(18, 18);
        closeButton->setStyleSheet(QStringLiteral(
            "QPushButton#RuntimePathsCloseButton { border: none; background: transparent; padding: 0; margin: 0; }"
            "QPushButton#RuntimePathsCloseButton:hover { background-color: #f5f5f5; border-radius: 4px; }"
        ));

        QIcon closeIcon(QStringLiteral(":/Resources/Icons/icons8-close-window-48.png"));
        if (!closeIcon.isNull())
        {
            closeButton->setIcon(closeIcon);
            closeButton->setIconSize(QSize(12, 12));
        }
        else
        {
            closeButton->setText(QString::fromUtf8("\u00D7"));
        }
        layout->addWidget(closeButton, 0, Qt::AlignVCenter);

        connect(closeButton, &QPushButton::clicked, m_parentDialog, &QDialog::close);
        closeButton->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        auto* closeButton = findChild<QPushButton*>(QStringLiteral("RuntimePathsCloseButton"));
        if (closeButton && watched == closeButton)
        {
            if (event->type() == QEvent::Enter)
            {
                QIcon hoverIcon(QStringLiteral(":/Resources/Icons/icons8-close-window-48-hover.png"));
                if (!hoverIcon.isNull())
                {
                    closeButton->setIcon(hoverIcon);
                }
            }
            else if (event->type() == QEvent::Leave)
            {
                QIcon normalIcon(QStringLiteral(":/Resources/Icons/icons8-close-window-48.png"));
                if (!normalIcon.isNull())
                {
                    closeButton->setIcon(normalIcon);
                }
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255));

        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        qreal r = 11.0;
        path.addRoundedRect(rect(), r, r);
        path.addRect(0, r, width(), height() - r);
        painter.drawPath(path.simplified());

        painter.setPen(QPen(QColor(0xd9, 0xe1, 0xea), 1));
        painter.drawLine(0, height() - 1, width(), height() - 1);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            m_dragPosition = event->globalPosition().toPoint() - m_parentDialog->frameGeometry().topLeft();
            m_isDragging = true;
            event->accept();
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_isDragging && (event->buttons() & Qt::LeftButton))
        {
            m_parentDialog->move(event->globalPosition().toPoint() - m_dragPosition);
            event->accept();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            m_isDragging = false;
            event->accept();
        }
    }

private:
    QDialog* m_parentDialog = nullptr;
    QPoint m_dragPosition;
    bool m_isDragging = false;
};

class RuntimePathsContentWidget final : public QWidget
{
public:
    explicit RuntimePathsContentWidget(QWidget* parent = nullptr) : QWidget(parent) {}
protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        qreal r = 11.0;
        path.addRoundedRect(rect(), r, r);
        path.addRect(0, 0, width(), height() - r);
        painter.setClipPath(path.simplified());

        painter.fillRect(rect(), Qt::white);
    }
};

} // namespace

RuntimePathsDialog::RuntimePathsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_TranslucentBackground, true);

    setWindowTitle(tr("Runtime Paths"));
    setMinimumWidth(640);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(1, 1, 1, 1);
    rootLayout->setSpacing(0);

    auto* titleBar = new RuntimePathsTitleBar(this, tr("Runtime Paths"));
    rootLayout->addWidget(titleBar);

    auto* contentWidget = new RuntimePathsContentWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(16, 12, 16, 12);
    contentLayout->setSpacing(12);
    rootLayout->addWidget(contentWidget);

    auto* description = new QLabel(
        tr("Configure OpenCV filter script paths. Empty saved overrides are filled from auto-detected compiler/OpenCV paths."),
        contentWidget);
    description->setWordWrap(true);
    description->setObjectName(QStringLiteral("RuntimePathsDescription"));
    contentLayout->addWidget(description);

    auto* formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    _compilerEdit = addPathRow(tr("C++ Compiler"), false, contentWidget);
    _includeEdit = addPathRow(tr("OpenCV Include Dir"), true, contentWidget);
    _libraryDirEdit = addPathRow(tr("OpenCV Library Dir"), true, contentWidget);
    _librariesEdit = new QLineEdit(contentWidget);
    _librariesEdit->setPlaceholderText(QStringLiteral("opencv_core;opencv_imgproc"));

    formLayout->addRow(tr("C++ Compiler"), _compilerEdit->parentWidget());
    formLayout->addRow(tr("OpenCV Include Dir"), _includeEdit->parentWidget());
    formLayout->addRow(tr("OpenCV Library Dir"), _libraryDirEdit->parentWidget());
    formLayout->addRow(tr("Libraries"), _librariesEdit);
    contentLayout->addLayout(formLayout);

    _statusLabel = new QLabel(contentWidget);
    _statusLabel->setObjectName(QStringLiteral("RuntimePathsStatusLabel"));
    _statusLabel->setProperty("state", QStringLiteral("normal"));
    _statusLabel->setWordWrap(true);
    contentLayout->addWidget(_statusLabel);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, contentWidget);
    auto* clearButton = buttonBox->addButton(tr("Clear OpenCV Overrides"), QDialogButtonBox::ResetRole);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        clearSettings();
        loadSettings();
        updateStatus();
    });
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        saveSettings();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    contentLayout->addWidget(buttonBox);

    connect(_compilerEdit, &QLineEdit::textChanged, this, [this]() { updateStatus(); });
    connect(_includeEdit, &QLineEdit::textChanged, this, [this]() { updateStatus(); });
    connect(_libraryDirEdit, &QLineEdit::textChanged, this, [this]() { updateStatus(); });

    loadSettings();
    updateStatus();
}

void RuntimePathsDialog::loadSettings() {
    QSettings settings;
    const OpenCvBuildEnvironment detected = OpenCvBuildEnvironment::fromBuildDefaults();
    _compilerEdit->setText(settings.value(QString::fromLatin1(kCompilerPathKey), detected.compilerPath).toString());
    _includeEdit->setText(settings.value(QString::fromLatin1(kIncludeDirKey), detected.includeDir).toString());
    _libraryDirEdit->setText(settings.value(QString::fromLatin1(kLibraryDirKey), detected.libraryDir).toString());
    _librariesEdit->setText(settings.value(QString::fromLatin1(kLibrariesKey), detected.libraries.join(QStringLiteral(";"))).toString());
}

void RuntimePathsDialog::saveSettings() {
    QSettings settings;
    const auto saveOrRemove = [&settings](const char* key, const QString& value) {
        const QString settingKey = QString::fromLatin1(key);
        const QString trimmed = value.trimmed();
        if (trimmed.isEmpty()) {
            settings.remove(settingKey);
        } else {
            settings.setValue(settingKey, trimmed);
        }
    };
    saveOrRemove(kCompilerPathKey, _compilerEdit->text());
    saveOrRemove(kIncludeDirKey, _includeEdit->text());
    saveOrRemove(kLibraryDirKey, _libraryDirEdit->text());
    saveOrRemove(kLibrariesKey, _librariesEdit->text());
}

void RuntimePathsDialog::clearSettings() {
    QSettings settings;
    settings.remove(QStringLiteral("RuntimePaths/OpenCV"));
}

void RuntimePathsDialog::updateStatus() {
    QStringList warnings;
    const QString compilerPath = _compilerEdit->text().trimmed();
    const QString includeDir = _includeEdit->text().trimmed();
    const QString libraryDir = _libraryDirEdit->text().trimmed();

    if (!compilerPath.isEmpty() && !QFileInfo(compilerPath).isFile()) {
        warnings << tr("C++ compiler path does not exist as a file.");
    }
    const QString compilerName = QFileInfo(compilerPath).fileName();
    if (compilerName == QStringLiteral("clang")
        || compilerName == QStringLiteral("gcc")
        || compilerName == QStringLiteral("cc")) {
        warnings << tr("Use a C++ compiler driver such as clang++, g++, or c++.");
    }
    if (!includeDir.isEmpty() && !QFileInfo(includeDir).isDir()) {
        warnings << tr("OpenCV include directory does not exist.");
    }
    if (!libraryDir.isEmpty() && !QFileInfo(libraryDir).isDir()) {
        warnings << tr("OpenCV library directory does not exist.");
    }

    if (warnings.isEmpty()) {
        _statusLabel->setText(tr("These values are used by OpenCV filter script compilation. Clear overrides reloads auto-detected defaults."));
        _statusLabel->setProperty("state", QStringLiteral("normal"));
    } else {
        _statusLabel->setText(warnings.join(QStringLiteral(" ")));
        _statusLabel->setProperty("state", QStringLiteral("warning"));
    }
    _statusLabel->style()->unpolish(_statusLabel);
    _statusLabel->style()->polish(_statusLabel);
}

QLineEdit* RuntimePathsDialog::addPathRow(const QString& label, bool directory, QWidget* parentWidget) {
    auto* container = new QWidget(parentWidget);
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto* edit = new QLineEdit(container);
    auto* browseButton = new QPushButton(tr("Browse..."), container);
    connect(browseButton, &QPushButton::clicked, this, [this, edit, directory, label]() {
        QString selected;
        if (directory) {
            selected = QFileDialog::getExistingDirectory(this, label, edit->text());
        } else {
            selected = QFileDialog::getOpenFileName(this, label, edit->text());
        }
        if (!selected.isEmpty()) {
            edit->setText(selected);
        }
    });

    layout->addWidget(edit, 1);
    layout->addWidget(browseButton);
    return edit;
}

void RuntimePathsDialog::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Draw smooth rounded white background with light border #d9e1ea
    painter.setPen(QPen(QColor(0xd9, 0xe1, 0xea), 1));
    painter.setBrush(QBrush(Qt::white));

    QRectF r = rect();
    r.adjust(0.5, 0.5, -0.5, -0.5);
    painter.drawRoundedRect(r, 12, 12);
}
