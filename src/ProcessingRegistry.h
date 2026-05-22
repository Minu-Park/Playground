#pragma once
#include <QString>
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <mutex>
#include "ProcessingPipeline.h"

struct NodeDefinition {
    QString id;
    QString displayName;
    QString description;
};

class ProcessingRegistry {
public:
    using NodeFactory = std::function<std::shared_ptr<ProcessingNode>()>;

    static ProcessingRegistry& instance();

    void registerNode(const NodeDefinition& def, NodeFactory factory);
    std::vector<NodeDefinition> availableNodes() const;
    std::shared_ptr<ProcessingNode> createNode(const QString& id) const;

private:
    ProcessingRegistry();
    ~ProcessingRegistry();
    ProcessingRegistry(const ProcessingRegistry&) = delete;
    ProcessingRegistry& operator=(const ProcessingRegistry&) = delete;

    struct RegistryEntry {
        NodeDefinition definition;
        NodeFactory factory;
    };

    std::map<QString, RegistryEntry> _entries;
    mutable std::mutex _mutex;
};
