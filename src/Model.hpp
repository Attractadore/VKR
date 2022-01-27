#pragma once
#include "VKR.hpp"

namespace VKR {
struct StaticModel {
    MeshID mesh;
    MaterialID material;
    glm::mat4 transform;

    void create(
        MeshID mesh_id, MaterialID material_id, const glm::mat4& t
    ) {
        mesh = mesh_id;
        material = material_id;
        transform = t;
    }

    void destroy() {}
};

struct DynamicModel {
    MeshID mesh;
    MaterialID material;
    glm::mat4 transform;

    void create(
        MeshID mesh_id, MaterialID material_id, const glm::mat4& t
    ) {
        mesh = mesh_id;
        material = material_id;
        transform = t;
    }

    void destroy() {}
};
};
