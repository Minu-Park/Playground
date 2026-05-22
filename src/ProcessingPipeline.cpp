#include "ProcessingPipeline.h"

void ProcessingPipeline::addNode(std::shared_ptr<ProcessingNode> node) {
    if (!node) return;
    std::lock_guard<std::mutex> lock(_mutex);
    _nodes.push_back(std::move(node));
}

void ProcessingPipeline::clearNodes() {
    std::lock_guard<std::mutex> lock(_mutex);
    _nodes.clear();
}

void ProcessingPipeline::run(ProcessingFrame& frame) {
    std::vector<std::shared_ptr<ProcessingNode>> nodesCopy;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        nodesCopy = _nodes;
    }
    
    for (auto& node : nodesCopy) {
        if (node && node->isEnabled()) {
            node->process(frame);
        }
    }
}
