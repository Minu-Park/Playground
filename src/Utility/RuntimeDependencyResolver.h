#pragma once

#include <QString>
#include <QStringList>

struct OpenCvRuntimePaths {
    QString compilerPath;
    QString includeDir;
    QString libraryDir;
    QStringList libraries;
};

class RuntimeDependencyResolver {
public:
    static OpenCvRuntimePaths resolveOpenCv(const OpenCvRuntimePaths& defaults);

private:
    static QString settingValue(const QString& key);
    static QString environmentValue(const char* key);
    static QString appRuntimePath(const QString& dependency, const QString& child);
    static QString firstPath(const QStringList& candidates);
    static QString firstValue(const QStringList& candidates);
    static QStringList splitList(const QString& value);
};
