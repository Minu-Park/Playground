#include "Pipeline/DynamicProcessingCompiler.h"
#include "Utility/RuntimeDependencyResolver.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace {

QString normalizedCxxCompilerPath(const QString& compilerPath) {
    if (compilerPath.trimmed().isEmpty()) {
        return compilerPath;
    }

    const QFileInfo compilerInfo(compilerPath);
    const QString fileName = compilerInfo.fileName();
    QString replacementName;
    if (fileName == QStringLiteral("clang")) {
        replacementName = QStringLiteral("clang++");
    } else if (fileName == QStringLiteral("gcc")) {
        replacementName = QStringLiteral("g++");
    } else if (fileName == QStringLiteral("cc")) {
        replacementName = QStringLiteral("c++");
    }

    if (replacementName.isEmpty()) {
        return compilerPath;
    }

    const QString siblingPath = compilerInfo.dir().filePath(replacementName);
    if (QFileInfo(siblingPath).isFile()) {
        return siblingPath;
    }
    return compilerPath;
}

bool looksLikeCCompilerDriver(const QString& compilerPath) {
    const QString fileName = QFileInfo(compilerPath).fileName();
    return fileName == QStringLiteral("clang")
        || fileName == QStringLiteral("gcc")
        || fileName == QStringLiteral("cc");
}

} // namespace

#ifndef PLAYGROUND_CXX_COMPILER
#define PLAYGROUND_CXX_COMPILER ""
#endif

#ifndef PLAYGROUND_CXX_COMPILER_ID
#define PLAYGROUND_CXX_COMPILER_ID ""
#endif

#ifndef PLAYGROUND_SHARED_LIBRARY_SUFFIX
#define PLAYGROUND_SHARED_LIBRARY_SUFFIX ""
#endif

#ifndef PLAYGROUND_SRC_DIR
#define PLAYGROUND_SRC_DIR ""
#endif

#ifndef OPENCV_INCLUDE_DIR
#define OPENCV_INCLUDE_DIR ""
#endif

#ifndef OPENCV_LIB_DIR
#define OPENCV_LIB_DIR ""
#endif

OpenCvBuildEnvironment OpenCvBuildEnvironment::fromBuildDefaults() {
    OpenCvBuildEnvironment environment;
    environment.compilerPath = QString::fromLocal8Bit(PLAYGROUND_CXX_COMPILER);
    environment.compilerId = QString::fromLocal8Bit(PLAYGROUND_CXX_COMPILER_ID);
    environment.sharedLibrarySuffix = QString::fromLocal8Bit(PLAYGROUND_SHARED_LIBRARY_SUFFIX);
    if (environment.sharedLibrarySuffix.isEmpty()) {
        environment.sharedLibrarySuffix = QStringLiteral(".so");
    }
    environment.includeDir = QString::fromLocal8Bit(OPENCV_INCLUDE_DIR);
    environment.libraryDir = QString::fromLocal8Bit(OPENCV_LIB_DIR);
    environment.libraries = {QStringLiteral("opencv_core"), QStringLiteral("opencv_imgproc")};
    environment.projectIncludeDir = QString::fromLocal8Bit(PLAYGROUND_SRC_DIR);

    OpenCvRuntimePaths defaults;
    defaults.compilerPath = environment.compilerPath;
    defaults.includeDir = environment.includeDir;
    defaults.libraryDir = environment.libraryDir;
    defaults.libraries = environment.libraries;

    const OpenCvRuntimePaths resolved = RuntimeDependencyResolver::resolveOpenCv(defaults);
    environment.compilerPath = normalizedCxxCompilerPath(resolved.compilerPath);
    environment.includeDir = resolved.includeDir;
    environment.libraryDir = resolved.libraryDir;
    environment.libraries = resolved.libraries;
    return environment;
}

bool OpenCvBuildEnvironment::isMsvc() const {
    return compilerId.contains(QStringLiteral("MSVC"), Qt::CaseInsensitive)
        || compilerPath.endsWith(QStringLiteral("cl.exe"), Qt::CaseInsensitive)
        || compilerPath == QStringLiteral("cl");
}

bool OpenCvBuildEnvironment::validate(QString* error) const {
    if (compilerPath.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Compiler path is empty.");
        return false;
    }
    if (!isMsvc() && looksLikeCCompilerDriver(compilerPath)) {
        if (error) *error = QStringLiteral("Compiler path must point to a C++ compiler driver such as clang++, g++, or c++.");
        return false;
    }
    if (includeDir.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("OpenCV include path is empty.");
        return false;
    }
    if (!QFileInfo(includeDir).isDir()) {
        if (error) *error = QStringLiteral("OpenCV include path does not exist: %1").arg(includeDir);
        return false;
    }
    if (libraryDir.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("OpenCV library path is empty.");
        return false;
    }
    if (!QFileInfo(libraryDir).isDir()) {
        if (error) *error = QStringLiteral("OpenCV library path does not exist: %1").arg(libraryDir);
        return false;
    }
    return true;
}

bool DynamicProcessingCompiler::prepareOpenCvProcessImageBuild(
    const OpenCvBuildEnvironment& environment,
    const DynamicProcessingCompileRequest& request,
    DynamicProcessingCompilePlan* plan,
    QString* error) {
    if (!plan) {
        if (error) *error = QStringLiteral("Build plan output is null.");
        return false;
    }
    if (!environment.validate(error)) {
        return false;
    }
    if (request.scratchDir.trimmed().isEmpty()) {
        if (error) *error = QStringLiteral("Scratch directory is empty.");
        return false;
    }
    if (!QDir().mkpath(request.scratchDir)) {
        if (error) *error = QStringLiteral("Failed to create scratch directory: %1").arg(request.scratchDir);
        return false;
    }

    const QString sourceFileName = QStringLiteral("dynamic_filter.cpp");
    const QString sourcePath = QDir(request.scratchDir).filePath(sourceFileName);
    if (!writeOpenCvProcessImageSource(request, sourcePath, error)) {
        return false;
    }

    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    const QString outputFileName = environment.isMsvc()
        ? QStringLiteral("dynamic_filter_%1%2").arg(timestamp).arg(environment.sharedLibrarySuffix)
        : QStringLiteral("libdynamic_filter_%1%2").arg(timestamp).arg(environment.sharedLibrarySuffix);
    const QString outputPath = QDir(request.scratchDir).filePath(outputFileName);

    QStringList arguments;
    if (environment.isMsvc()) {
        arguments << QStringLiteral("/nologo")
                  << QStringLiteral("/O2")
                  << QStringLiteral("/std:c++20")
                  << QStringLiteral("/LD")
                  << sourceFileName
                  << QStringLiteral("/I") + environment.includeDir;
        if (!environment.projectIncludeDir.isEmpty()) {
            arguments << QStringLiteral("/I") + environment.projectIncludeDir;
        }
        if (!environment.libraryDir.isEmpty()) {
            arguments << QStringLiteral("/link") << QStringLiteral("/LIBPATH:") + environment.libraryDir;
        }
        for (const QString& library : environment.libraries) {
            arguments << (library.endsWith(QStringLiteral(".lib"), Qt::CaseInsensitive)
                ? library
                : library + QStringLiteral(".lib"));
        }
        arguments << QStringLiteral("/OUT:") + outputFileName;
    } else {
        arguments << QStringLiteral("-O2")
                  << QStringLiteral("-shared")
                  << QStringLiteral("-fPIC")
                  << QStringLiteral("-std=c++20")
                  << QStringLiteral("-I") + environment.includeDir;
        if (!environment.projectIncludeDir.isEmpty()) {
            arguments << QStringLiteral("-I") + environment.projectIncludeDir;
        }
        if (!environment.libraryDir.isEmpty()) {
            arguments << QStringLiteral("-L") + environment.libraryDir;
            arguments << QStringLiteral("-Wl,-rpath,") + environment.libraryDir;
        }
        for (const QString& library : environment.libraries) {
            arguments << (library.startsWith(QStringLiteral("-l")) ? library : QStringLiteral("-l") + library);
        }
        arguments << sourceFileName
                  << QStringLiteral("-o")
                  << outputFileName;
    }

    plan->workingDirectory = request.scratchDir;
    plan->sourcePath = sourcePath;
    plan->outputPath = outputPath;
    plan->outputFileName = outputFileName;
    plan->compilerPath = environment.compilerPath;
    plan->arguments = arguments;
    return true;
}

bool DynamicProcessingCompiler::writeOpenCvProcessImageSource(
    const DynamicProcessingCompileRequest& request,
    const QString& sourcePath,
    QString* error) {
    QFile file(sourcePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = QStringLiteral("Failed to create dynamic filter source: %1").arg(sourcePath);
        return false;
    }

    QTextStream out(&file);
    out << "#include <opencv2/opencv.hpp>\n"
        << "#include <algorithm>\n\n"
        << "extern \"C\" {\n"
        << "    void process_image(unsigned char* data, int width, int height, int channels, int step, const double* params, int paramCount) {\n";

    for (size_t i = 0; i < request.parameters.size(); ++i) {
        const auto& parameter = request.parameters[i];
        out << "        double " << parameter.varName << " = (paramCount > " << i << ") ? params[" << i << "] : " << parameter.defaultValue << ";\n";
    }

    out << "\n"
        << "        int type = (channels == 4) ? CV_8UC4 : ((channels == 3) ? CV_8UC3 : CV_8UC1);\n"
        << "        cv::Mat img(height, width, type, data, step);\n\n"
        << "        // USER SCRIPT START\n"
        << request.scriptCode << "\n"
        << "        // USER SCRIPT END\n"
        << "    }\n"
        << "}\n";
    return true;
}
