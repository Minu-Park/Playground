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

namespace {

constexpr auto kCompilerPathKey = "RuntimePaths/OpenCV/CompilerPath";
constexpr auto kIncludeDirKey = "RuntimePaths/OpenCV/IncludeDir";
constexpr auto kLibraryDirKey = "RuntimePaths/OpenCV/LibraryDir";
constexpr auto kLibrariesKey = "RuntimePaths/OpenCV/Libraries";

} // namespace

RuntimePathsDialog::RuntimePathsDialog(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Runtime Paths"));
    setMinimumWidth(640);

    auto* rootLayout = new QVBoxLayout(this);

    auto* description = new QLabel(
        tr("Configure OpenCV filter script paths. Empty saved overrides are filled from auto-detected compiler/OpenCV paths."),
        this);
    description->setWordWrap(true);
    description->setObjectName(QStringLiteral("RuntimePathsDescription"));
    rootLayout->addWidget(description);

    auto* formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    _compilerEdit = addPathRow(tr("C++ Compiler"), false);
    _includeEdit = addPathRow(tr("OpenCV Include Dir"), true);
    _libraryDirEdit = addPathRow(tr("OpenCV Library Dir"), true);
    _librariesEdit = new QLineEdit(this);
    _librariesEdit->setPlaceholderText(QStringLiteral("opencv_core;opencv_imgproc"));

    formLayout->addRow(tr("C++ Compiler"), _compilerEdit->parentWidget());
    formLayout->addRow(tr("OpenCV Include Dir"), _includeEdit->parentWidget());
    formLayout->addRow(tr("OpenCV Library Dir"), _libraryDirEdit->parentWidget());
    formLayout->addRow(tr("Libraries"), _librariesEdit);
    rootLayout->addLayout(formLayout);

    _statusLabel = new QLabel(this);
    _statusLabel->setObjectName(QStringLiteral("RuntimePathsStatusLabel"));
    _statusLabel->setProperty("state", QStringLiteral("normal"));
    _statusLabel->setWordWrap(true);
    rootLayout->addWidget(_statusLabel);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
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
    rootLayout->addWidget(buttonBox);

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

QLineEdit* RuntimePathsDialog::addPathRow(const QString& label, bool directory) {
    auto* container = new QWidget(this);
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
