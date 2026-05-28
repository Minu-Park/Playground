#pragma once
#include "Pipeline/ProcessingFrame.h"
#include <QString>
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>

struct ParameterSpec {
    QString name;
    double minValue = 0.0;
    double maxValue = 100.0;
    double defaultValue = 0.0;
    int decimals = 0; // 0: integer slider, >=1: double spinbox
};

class ProcessingNode {
public:
    virtual ~ProcessingNode() = default;
    virtual QString name() const = 0;
    virtual void process(ProcessingFrame& frame) = 0;
    
    virtual bool isEnabled() const { return _enabled.load(std::memory_order_acquire); }
    virtual void setEnabled(bool on) { _enabled.store(on, std::memory_order_release); }

    virtual std::vector<ParameterSpec> parameterSpecs() const { return {}; }
    virtual void setParameter(int index, double value) { (void)index; (void)value; }
    virtual double getParameter(int index) const { (void)index; return 0.0; }

private:
    std::atomic_bool _enabled{true};
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
