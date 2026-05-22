#pragma once
#include "ProcessingFrame.h"
#include <QString>
#include <memory>
#include <vector>
#include <mutex>

class ProcessingNode {
public:
    virtual ~ProcessingNode() = default;
    virtual QString name() const = 0;
    virtual void process(ProcessingFrame& frame) = 0;
    
    virtual bool isEnabled() const { return _enabled; }
    virtual void setEnabled(bool on) { _enabled = on; }
    virtual void setParameter(int index, double value) { (void)index; (void)value; }

private:
    bool _enabled = true;
};

class ProcessingPipeline {
public:
    ProcessingPipeline() = default;
    ~ProcessingPipeline() = default;

    void addNode(std::shared_ptr<ProcessingNode> node);
    void clearNodes();
    void run(ProcessingFrame& frame);

private:
    std::vector<std::shared_ptr<ProcessingNode>> _nodes;
    std::mutex _mutex;
};
