#include "Utility/RuntimeDependencyResolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>

OpenCvRuntimePaths RuntimeDependencyResolver::resolveOpenCv(const OpenCvRuntimePaths& defaults) {
    OpenCvRuntimePaths resolved;

    resolved.compilerPath = firstValue({
        settingValue(QStringLiteral("RuntimePaths/OpenCV/CompilerPath")),
        environmentValue("PLAYGROUND_OPENCV_COMPILER"),
        defaults.compilerPath
    });

    resolved.includeDir = firstPath({
        settingValue(QStringLiteral("RuntimePaths/OpenCV/IncludeDir")),
        environmentValue("PLAYGROUND_OPENCV_INCLUDE_DIR"),
        appRuntimePath(QStringLiteral("opencv"), QStringLiteral("include")),
        appRuntimePath(QStringLiteral("OpenCV"), QStringLiteral("include")),
        defaults.includeDir
    });

    resolved.libraryDir = firstPath({
        settingValue(QStringLiteral("RuntimePaths/OpenCV/LibraryDir")),
        environmentValue("PLAYGROUND_OPENCV_LIB_DIR"),
        appRuntimePath(QStringLiteral("opencv"), QStringLiteral("lib")),
        appRuntimePath(QStringLiteral("OpenCV"), QStringLiteral("lib")),
        defaults.libraryDir
    });

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
