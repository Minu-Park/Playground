#pragma once

#include <QDialog>

class QLineEdit;
class QLabel;

class RuntimePathsDialog : public QDialog {
public:
    explicit RuntimePathsDialog(QWidget* parent = nullptr);

private:
    void loadSettings();
    void saveSettings();
    void clearSettings();
    void updateStatus();
    QLineEdit* addPathRow(const QString& label, bool directory);

    QLineEdit* _compilerEdit = nullptr;
    QLineEdit* _includeEdit = nullptr;
    QLineEdit* _libraryDirEdit = nullptr;
    QLineEdit* _librariesEdit = nullptr;
    QLabel* _statusLabel = nullptr;
};
