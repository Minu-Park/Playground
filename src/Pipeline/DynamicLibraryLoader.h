#pragma once
#include <QLibrary>
#include <memory>
#include <QString>
#include <mutex>

class DynamicLibraryLoader {
public:
    static std::shared_ptr<DynamicLibraryLoader> load(const QString& path);
    ~DynamicLibraryLoader();

    void* resolve(const char* symbol);

private:
    explicit DynamicLibraryLoader(const QString& path);
    
    QString _path;
    QLibrary _library;
    mutable std::mutex _mutex;
};
