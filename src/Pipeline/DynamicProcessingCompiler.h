#pragma once

#include "Utility/ParameterParser.h"
#include <QString>
#include <QStringList>
#include <vector>

struct OpenCvBuildEnvironment {
    QString compilerPath;
    QString compilerId;
    QString sharedLibrarySuffix;
    QString includeDir;
    QString libraryDir;
    QStringList libraries;
    QString projectIncludeDir;

    static OpenCvBuildEnvironment fromBuildDefaults();
    bool isMsvc() const;
    bool validate(QString* error) const;
};

struct DynamicProcessingCompileRequest {
    QString scriptCode;
    std::vector<ParsedParameter> parameters;
    QString scratchDir;
};

struct DynamicProcessingCompilePlan {
    QString workingDirectory;
    QString sourcePath;
    QString outputPath;
    QString outputFileName;
    QString compilerPath;
    QStringList arguments;
};

class DynamicProcessingCompiler {
public:
    static bool prepareOpenCvProcessImageBuild(
        const OpenCvBuildEnvironment& environment,
        const DynamicProcessingCompileRequest& request,
        DynamicProcessingCompilePlan* plan,
        QString* error);

private:
    static bool writeOpenCvProcessImageSource(
        const DynamicProcessingCompileRequest& request,
        const QString& sourcePath,
        QString* error);
};
