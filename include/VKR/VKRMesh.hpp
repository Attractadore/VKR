#pragma once
#include "VKRTypes.hpp"

#include <glm/mat4x4.hpp>

#include <span>

namespace VKR {
class Mesh;
class StaticMesh;
class StreamingMesh;
class Model;

StaticMesh* createStaticMesh(std::span<const VKRTriangle> data);
void destroyStaticMesh(StaticMesh* mesh);

StreamingMesh* createStreamingMesh(unsigned num_reserved_triangles);
void destroyStreamingMesh(StreamingMesh* mesh);
void updateStreamingMesh(StreamingMesh*, std::span<const VKRTriangle> data); 

Model* createModel(const Mesh* mesh);
void destroyModel(Model* model);
void modelSetMatrix(Model* model, const glm::mat4& matrix);
}
