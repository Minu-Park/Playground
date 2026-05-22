#include "DynamicLibraryLoader.h"
#include <QDebug>

// DynamicLibraryNode implementation
DynamicLibraryNode::DynamicLibraryNode(std::shared_ptr<DynamicLibraryLoader> loader, ProcessingNode* rawNode)
    : _loader(std::move(loader)), _rawNode(rawNode) {}

DynamicLibraryNode::~DynamicLibraryNode() {
    // Explicitly delete rawNode before loader shared_ptr goes out of scope.
    // This ensures that the node object is destroyed while the library memory is still mapped.
    _rawNode.reset();
}

QString DynamicLibraryNode::name() const {
    if (_rawNode) {
        return _rawNode->name();
    }
    return "Unknown Dynamic Node";
}

void DynamicLibraryNode::process(ProcessingFrame& frame) {
    if (_rawNode) {
        _rawNode->process(frame);
    }
}

void DynamicLibraryNode::setParameter(int index, double value) {
    if (_rawNode) {
        _rawNode->setParameter(index, value);
    }
}

// DynamicLibraryLoader implementation
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

bool DynamicLibraryLoader::isLoaded() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _library.isLoaded();
}

std::shared_ptr<ProcessingNode> DynamicLibraryLoader::createNode() {
    using CreateNodeFunc = ProcessingNode* (*)();
    auto func = reinterpret_cast<CreateNodeFunc>(resolve("create_node"));
    if (!func) {
        qWarning() << "[DynamicLibraryLoader] Could not resolve 'create_node' symbol in library:" << _path;
        return nullptr;
    }
    
    ProcessingNode* rawNode = func();
    if (!rawNode) {
        qWarning() << "[DynamicLibraryLoader] 'create_node' returned nullptr in library:" << _path;
        return nullptr;
    }
    
    return std::make_shared<DynamicLibraryNode>(shared_from_this(), rawNode);
}
