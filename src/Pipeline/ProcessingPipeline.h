#pragma once
#include "Pipeline/ProcessingFrame.h"
#include <QString>
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
    
    virtual bool isEnabled() const { return _enabled; }
    virtual void setEnabled(bool on) { _enabled = on; }

    virtual std::vector<ParameterSpec> parameterSpecs() const { return {}; }
    virtual void setParameter(int index, double value) { (void)index; (void)value; }
    virtual double getParameter(int index) const { (void)index; return 0.0; }

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
