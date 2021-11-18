#include "VKRMemory.hpp"
#include "VKRMeshInternal.hpp"
#include "VKRState.hpp"
#include "VKRSynchronization.hpp"
#include "VKRUtils.hpp"

#include <span>

namespace VKR {
StreamingMesh::StreamingMesh(unsigned num_reserved_triangles):
    frame{0}, first_vertex{0}, fences{0} {
    VKR_CreateHostBuffer(
        sizeof(VKRTriangle[num_streaming_frames]) * num_reserved_triangles, &buffer
    );
    void* mapped;
    vkMapMemory(vkr.device, buffer.mem, 0, VK_WHOLE_SIZE, 0, &mapped);
    mapped_data = static_cast<VKRTriangle*>(mapped);
    frame = 0;
    first_vertex = 0;
    num_vertices = 3 * num_reserved_triangles;
}

StreamingMesh::~StreamingMesh() {
    VKR_FreeBuffer(&buffer);
}

void StreamingMesh::update(std::span<const VKRTriangle> data) {
    assert(data.size() <= maxFrameTriangles());
    frame = (frame + 1) % num_streaming_frames;
    if (fences[frame]) {
        vkWaitForFences(
            vkr.device, 1, &fences[frame],
            true, std::numeric_limits<uint64_t>::max()
        );
    }
    unsigned first_triangle = getFirstVertex() / 3;
    std::copy(data.begin(), data.end(), mapped_data + first_triangle);
    num_vertices = 3 * data.size();
}

void StreamingMesh::insertFence(VkFence fence) {
    if (fences[frame]) {
        vkWaitForFences(
            vkr.device, 1, &fences[frame],
            true, std::numeric_limits<uint64_t>::max()
        );
    }
    fences[frame] = fence;
}

StaticMesh* createStaticMesh(std::span<const VKRTriangle> data) {
    vkr.static_meshes.emplace_back(std::make_unique<StaticMesh>(data));
    return vkr.static_meshes.back().get();
}

void destroyStaticMesh(StaticMesh* mesh) {
    auto it = std::find_if(
        vkr.static_meshes.begin(), vkr.static_meshes.end(),
        [&](const auto& up) {
            return up.get() == mesh;
        }
    );
    if (it != vkr.static_meshes.end()) {
        // TODO: Add a deletion queue
        vkDeviceWaitIdle(vkr.device);
        vkr.static_meshes.erase(it);
    }
}

StreamingMesh* createStreamingMesh(unsigned num_reserved_triangles) {
    vkr.streaming_meshes.push_back(std::make_unique<StreamingMesh>(num_reserved_triangles));
    return vkr.streaming_meshes.back().get();
}

void destroyStreamingMesh(StreamingMesh* mesh) {
    auto it = std::find_if(
        vkr.streaming_meshes.begin(), vkr.streaming_meshes.end(),
        [&](const auto& up) {
            return up.get() == mesh;
        }
    );
    if (it != vkr.streaming_meshes.end()) {
        // TODO: Add a deletion queue
        vkDeviceWaitIdle(vkr.device);
        vkr.streaming_meshes.erase(it);
    }
}

void updateStreamingMesh(StreamingMesh* mesh, std::span<const VKRTriangle> data) {
    mesh->update(data);
}

Model* createModel(const Mesh* mesh) {
    vkr.models.emplace_back(std::make_unique<Model>(mesh));
    return vkr.models.back().get();
}

void destroyModel(Model* model) {
    auto it = std::find_if(
        vkr.models.begin(), vkr.models.end(),
        [&](const auto& up) {
            return up.get() == model;
        }
    );
    if (it != vkr.models.end()) {
        vkr.models.erase(it);
    }
}

void modelSetMatrix(Model* model, const glm::mat4& matrix) {
    model->setModel(matrix);
}
}
