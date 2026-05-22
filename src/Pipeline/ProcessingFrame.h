#pragma once
#include <QImage>
#include "engine/GraphicsSceneTypes.h"
#include <variant>

struct ProcessingFrame {
    std::variant<QImage, GraphicsScene3D> payload;
    uint64_t frameSeq = 0;
};
