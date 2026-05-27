#include "Pipeline/DynamicLibraryLoader.h"
#include <QDebug>

DynamicLibraryLoader::DynamicLibraryLoader(const QString& path)
    : _path(path), _library(path) {}

DynamicLibraryLoader::~DynamicLibraryLoader() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_library.isLoaded()) {
        qDebug() << "[DynamicLibraryLoader] Safely unloading dynamic library:" << _path;
        _library.unload();
    }
}

std::shared_ptr<DynamicLibraryLoader> DynamicLibraryLoader::load(const QString& path) {
    // Since constructor is private, we use a helper structure or new
    struct Helper : public DynamicLibraryLoader {
        explicit Helper(const QString& p) : DynamicLibraryLoader(p) {}
    };
    
    auto loader = std::make_shared<Helper>(path);
    if (loader->_library.load()) {
        qDebug() << "[DynamicLibraryLoader] Successfully loaded dynamic library:" << path;
        return loader;
    }
    
    qWarning() << "[DynamicLibraryLoader] Failed to load library:" << path 
               << "Error:" << loader->_library.errorString();
    return nullptr;
}

void* DynamicLibraryLoader::resolve(const char* symbol) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_library.isLoaded()) {
        return (void*)_library.resolve(symbol);
    }
    return nullptr;
}
