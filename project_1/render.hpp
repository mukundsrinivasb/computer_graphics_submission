#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "model.hpp"

void drawMesh(const Mesh& mesh);
void renderNode(const ModelNode* node);
void drawFloor();

#endif
