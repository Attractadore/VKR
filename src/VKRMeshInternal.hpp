#pragma once
#include "VKRTypesInternal.hpp"
#include "VKRMesh.hpp"
#include "VKRMemory.hpp"

#include <array>

namespace VKR {
class Mesh {
protected:
    VKRBuffer buffer;
    unsigned num_vertices;

public:
    Mesh() {}
    Mesh(const Mesh& other) = delete;
    Mesh(Mesh&& other) {
        buffer = std::exchange(other.buffer, VKRBuffer{});
        num_vertices = std::exchange(other.num_vertices, 0);
    }

    Mesh& operator=(const Mesh& other) = delete;
    Mesh& operator=(Mesh&& other) {
        std::swap(buffer, other.buffer);
        std::swap(num_vertices, other.num_vertices);
        return *this;
    }

    virtual ~Mesh() = 0;

    VkBuffer getBuffer() const {
        return buffer.buf;
    }

    virtual unsigned getFirstVertex() const {
        return 0;
    }

    unsigned getNumVertices() const {
        return num_vertices;
    }
};

inline Mesh::~Mesh() {}

class StaticMesh final: public Mesh {
public:
    StaticMesh(std::span<const VKRTriangle> data) {
        VKR_CreateDeviceBuffer(
            data.data(), data.size_bytes(), &buffer, VK_NULL_HANDLE
        );
        num_vertices = 3 * data.size();
    }

    ~StaticMesh() {
        VKR_FreeBuffer(&buffer);
    }
};

class StreamingMesh final: public Mesh {
    static constexpr size_t num_streaming_frames = 4;

    std::array<VkFence, num_streaming_frames> fences;
    VKRTriangle* mapped_data;
    unsigned frame;
    unsigned first_vertex;

public:
    StreamingMesh(unsigned num_reserved_triangles);

    StreamingMesh(const StreamingMesh& other) = delete;
    StreamingMesh(StreamingMesh&& other) {
        fences = std::move(other.fences);
        other.fences.fill(VK_NULL_HANDLE);
        mapped_data = std::exchange(other.mapped_data, nullptr);
        frame = std::exchange(other.frame, 0);
        first_vertex = std::exchange(other.first_vertex, 0);
    }

    StreamingMesh& operator=(const StreamingMesh& other) = delete;
    StreamingMesh& operator=(StreamingMesh&& other) {
        std::swap(fences, other.fences);
        std::swap(mapped_data, other.mapped_data);
        std::swap(frame, other.frame);
        std::swap(first_vertex, other.first_vertex);
        return *this;
    }

    ~StreamingMesh();

    unsigned maxFrameTriangles() const {
        return buffer.size / sizeof(VKRTriangle[num_streaming_frames]);
    }

    unsigned maxFrameVertices() const {
        return 3 * maxFrameTriangles();
    }

    unsigned getFirstVertex() const override {
        return frame * maxFrameVertices();
    }

    void update(std::span<const VKRTriangle> data);

    void insertFence(VkFence fence);
};

class Model {
    const Mesh* mesh;

public:
    glm::mat4 model;

public:
    Model(const Mesh* m):
        mesh{m}, model{1.0f} {}

    auto getBuffer() const {
        return mesh->getBuffer();
    }

    auto getFirstVertex() const {
        return mesh->getFirstVertex();
    }

    auto getNumVertices() const {
        return mesh->getNumVertices();
    }

    void setModel(const glm::mat4& m) {
        model = m;
    }
};
}
