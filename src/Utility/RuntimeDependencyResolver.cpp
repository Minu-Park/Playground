#include "Utility/RuntimeDependencyResolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

OpenCvRuntimePaths RuntimeDependencyResolver::resolveOpenCv(const OpenCvRuntimePaths& defaults) {
    OpenCvRuntimePaths resolved;

    QStringList compilerCandidates = {
        settingValue(QStringLiteral("RuntimePaths/OpenCV/CompilerPath")),
        environmentValue("PLAYGROUND_OPENCV_COMPILER"),
        defaults.compilerPath
    };
    compilerCandidates.append(commonCxxCompilerCandidates());
    resolved.compilerPath = firstExecutable(compilerCandidates);

    QStringList includeCandidates = {
        settingValue(QStringLiteral("RuntimePaths/OpenCV/IncludeDir")),
        environmentValue("PLAYGROUND_OPENCV_INCLUDE_DIR"),
        appRuntimePath(QStringLiteral("opencv"), QStringLiteral("include")),
        appRuntimePath(QStringLiteral("OpenCV"), QStringLiteral("include")),
        defaults.includeDir
    };
    includeCandidates.append(commonOpenCvIncludeCandidates());
    resolved.includeDir = firstPath(includeCandidates);

    QStringList libraryCandidates = {
        settingValue(QStringLiteral("RuntimePaths/OpenCV/LibraryDir")),
        environmentValue("PLAYGROUND_OPENCV_LIB_DIR"),
        appRuntimePath(QStringLiteral("opencv"), QStringLiteral("lib")),
        appRuntimePath(QStringLiteral("OpenCV"), QStringLiteral("lib")),
        defaults.libraryDir
    };
    libraryCandidates.append(commonOpenCvLibraryCandidates());
    resolved.libraryDir = firstPath(libraryCandidates);

    resolved.libraries = splitList(settingValue(QStringLiteral("RuntimePaths/OpenCV/Libraries")));
    if (resolved.libraries.isEmpty()) {
        resolved.libraries = splitList(environmentValue("PLAYGROUND_OPENCV_LIBS"));
    }
    if (resolved.libraries.isEmpty()) {
        resolved.libraries = defaults.libraries;
    }

    return resolved;
}

QString RuntimeDependencyResolver::settingValue(const QString& key) {
    QSettings settings;
    return settings.value(key).toString().trimmed();
}

QString RuntimeDependencyResolver::environmentValue(const char* key) {
    return QString::fromLocal8Bit(qgetenv(key)).trimmed();
}

QString RuntimeDependencyResolver::appRuntimePath(const QString& dependency, const QString& child) {
    const QString appDir = QCoreApplication::applicationDirPath();
    if (appDir.isEmpty()) {
        return {};
    }
    return QDir(appDir).filePath(QStringLiteral("runtime/%1/%2").arg(dependency, child));
}

QStringList RuntimeDependencyResolver::commonCxxCompilerCandidates() {
    return {
        QStringLiteral("clang++"),
        QStringLiteral("c++"),
        QStringLiteral("g++"),
        QStringLiteral("cl.exe"),
        QStringLiteral("/usr/bin/clang++"),
        QStringLiteral("/usr/bin/c++"),
        QStringLiteral("/usr/bin/g++"),
        QStringLiteral("/opt/homebrew/opt/llvm/bin/clang++"),
        QStringLiteral("/opt/homebrew/bin/clang++"),
        QStringLiteral("/usr/local/bin/clang++"),
        QStringLiteral("/usr/local/bin/g++")
    };
}

QStringList RuntimeDependencyResolver::commonOpenCvIncludeCandidates() {
    return {
        QStringLiteral("/opt/opencv/include/opencv4"),
        QStringLiteral("/opt/homebrew/include/opencv4"),
        QStringLiteral("/usr/local/include/opencv4"),
        QStringLiteral("/usr/include/opencv4"),
        QStringLiteral("C:/opencv/build/include"),
        QStringLiteral("C:/OpenCV/build/include")
    };
}

QStringList RuntimeDependencyResolver::commonOpenCvLibraryCandidates() {
    return {
        QStringLiteral("/opt/opencv/lib"),
        QStringLiteral("/opt/homebrew/lib"),
        QStringLiteral("/usr/local/lib"),
        QStringLiteral("/usr/lib"),
        QStringLiteral("/usr/lib/x86_64-linux-gnu"),
        QStringLiteral("C:/opencv/build/x64/vc16/lib"),
        QStringLiteral("C:/OpenCV/build/x64/vc16/lib"),
        QStringLiteral("C:/opencv/build/x64/vc17/lib"),
        QStringLiteral("C:/OpenCV/build/x64/vc17/lib")
    };
}

QString RuntimeDependencyResolver::firstExecutable(const QStringList& candidates) {
    for (const QString& candidate : candidates) {
        if (candidate.trimmed().isEmpty()) {
            continue;
        }
        const QFileInfo info(candidate);
        if (info.isAbsolute() && info.isFile()) {
            return candidate;
        }
        const QString executable = QStandardPaths::findExecutable(candidate);
        if (!executable.isEmpty()) {
            return executable;
        }
    }
    return firstValue(candidates);
}

QString RuntimeDependencyResolver::firstPath(const QStringList& candidates) {
    for (const QString& candidate : candidates) {
        if (candidate.trimmed().isEmpty()) {
            continue;
        }
        if (QFileInfo(candidate).isDir()) {
            return candidate;
        }
    }
    return firstValue(candidates);
}

QString RuntimeDependencyResolver::firstValue(const QStringList& candidates) {
    for (const QString& candidate : candidates) {
        if (!candidate.trimmed().isEmpty()) {
            return candidate;
        }
    }
    return {};
}

QStringList RuntimeDependencyResolver::splitList(const QString& value) {
    if (value.trimmed().isEmpty()) {
        return {};
    }
    QStringList result = value.split(QRegularExpression(QStringLiteral("[,;]")), Qt::SkipEmptyParts);
    for (QString& item : result) {
        item = item.trimmed();
    }
    result.removeAll(QString());
    return result;
}
