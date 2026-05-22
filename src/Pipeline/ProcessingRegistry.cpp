#include "Pipeline/ProcessingRegistry.h"
#include <QImage>

namespace {
class InvertImageNode : public ProcessingNode {
public:
    QString name() const override { return "Invert Image"; }
    void process(ProcessingFrame& frame) override {
        if (std::holds_alternative<QImage>(frame.payload)) {
            QImage& img = std::get<QImage>(frame.payload);
            if (!img.isNull()) {
                // Perform a simple thread-safe pixel inversion for testing 2D image processing
                // Since this could be called from a grab thread, ensure we operate on a deep copy if shared.
                // QImage::invertPixels modifies the image in-place, and QImage uses implicit sharing.
                // To avoid modifying other copies, we make sure it is not shared.
                img.detach();
                img.invertPixels(QImage::InvertRgb);
            }
        }
    }
};
} // namespace

ProcessingRegistry& ProcessingRegistry::instance() {
    static ProcessingRegistry inst;
    return inst;
}

ProcessingRegistry::ProcessingRegistry() {
    // Register default built-in nodes
    registerNode(
        NodeDefinition{
            "invert_image",
            "Invert Image",
            "Inverts RGB pixels of 2D images (Qt-based, OpenCV-independent)."
        },
        []() { return std::make_shared<InvertImageNode>(); }
    );
}

ProcessingRegistry::~ProcessingRegistry() = default;

void ProcessingRegistry::registerNode(const NodeDefinition& def, NodeFactory factory) {
    if (def.id.isEmpty() || !factory) return;
    std::lock_guard<std::mutex> lock(_mutex);
    _entries[def.id] = RegistryEntry{def, std::move(factory)};
}

std::vector<NodeDefinition> ProcessingRegistry::availableNodes() const {
    std::lock_guard<std::mutex> lock(_mutex);
    std::vector<NodeDefinition> list;
    list.reserve(_entries.size());
    for (const auto& [id, entry] : _entries) {
        list.push_back(entry.definition);
    }
    return list;
}

std::shared_ptr<ProcessingNode> ProcessingRegistry::createNode(const QString& id) const {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _entries.find(id);
    if (it != _entries.end()) {
        return it->second.factory();
    }
    return nullptr;
}
