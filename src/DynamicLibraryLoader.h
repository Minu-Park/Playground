#pragma once
#include <QLibrary>
#include <memory>
#include <QString>
#include <mutex>
#include "ProcessingPipeline.h"

class DynamicLibraryNode : public ProcessingNode {
public:
    DynamicLibraryNode(std::shared_ptr<class DynamicLibraryLoader> loader, ProcessingNode* rawNode);
    ~DynamicLibraryNode() override;

    QString name() const override;
    void process(ProcessingFrame& frame) override;
    void setParameter(int index, double value) override;

private:
    std::shared_ptr<DynamicLibraryLoader> _loader;
    std::unique_ptr<ProcessingNode> _rawNode;
};

class DynamicLibraryLoader : public std::enable_shared_from_this<DynamicLibraryLoader> {
public:
    static std::shared_ptr<DynamicLibraryLoader> load(const QString& path);
    ~DynamicLibraryLoader();

    void* resolve(const char* symbol);
    bool isLoaded() const;
    
    // Helper to create node from this library
    std::shared_ptr<ProcessingNode> createNode();

private:
    explicit DynamicLibraryLoader(const QString& path);
    
    QString _path;
    QLibrary _library;
    mutable std::mutex _mutex;
};
