#pragma once
#include <glm/mat4x4.hpp>

#include <memory>
#include <span>
#include <string>

namespace VKR {
namespace Detail {
    using storageid = uint32_t;
    using meshid = uint32_t;
    using modelid = uint32_t;
    using materialid = uint32_t;
}

enum class MeshStorageFormat: Detail::storageid {
    Static,
    Dynamic,
    First = Static,
    Last = Dynamic,
};

enum MeshID: Detail::meshid;
enum MaterialID: Detail::materialid;
enum ModelID: Detail::modelid;

class Instance;
class GraphicsDevice;
class GraphicsDeviceConnection;

struct Camera;
class Scene;

namespace Vulkan {
class Instance;
class GraphicsDevice;
class GraphicsDeviceConnection;

class ISwapchain;
class IWSISwapchain;

class Scene;
};

class Instance {
protected:
    struct Impl;
    std::unique_ptr<Impl> pimpl;

public:
    Instance(std::span<const char* const> extensions);
    ~Instance();

    uint32_t getGraphicsDeviceCount() const;
    const GraphicsDevice& getGraphicsDevice(uint32_t dev_idx) const;

    Vulkan::Instance& vulkanAPI() {
        return reinterpret_cast<Vulkan::Instance&>(*this);
    }

    const Vulkan::Instance& vulkanAPI() const {
        return reinterpret_cast<const Vulkan::Instance&>(*this);
    }
};


struct GraphicsDeviceConnectionFeatures {
    bool present: 1;
};

class GraphicsDevice {
protected:
    GraphicsDevice() {}

public:
    std::string name() const;
    bool presentSupported() const;

    GraphicsDeviceConnection& createConnection(
        const GraphicsDeviceConnectionFeatures& conf 
    );

    Vulkan::GraphicsDevice& vulkanAPI() {
        return reinterpret_cast<Vulkan::GraphicsDevice&>(*this);
    }

    const Vulkan::GraphicsDevice& vulkanAPI() const {
        return reinterpret_cast<const Vulkan::GraphicsDevice&>(*this);
    }
};

class GraphicsDeviceConnection {
protected:
    GraphicsDeviceConnection() {}

public:
    Scene& createScene(
        const Camera& camera, uint32_t width, uint32_t height
    );

    Vulkan::GraphicsDeviceConnection& vulkanAPI() {
        return reinterpret_cast<Vulkan::GraphicsDeviceConnection&>(*this);
    }

    const Vulkan::GraphicsDeviceConnection& vulkanAPI() const {
        return reinterpret_cast<const Vulkan::GraphicsDeviceConnection&>(*this);
    }
};

struct Camera {
    float m_aspect_ratio;
    float m_hfov;
    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_up;
};

class Scene {
protected:
    Scene() {}

public:
    Camera m_camera;

public:
    MeshID createMesh(
        MeshStorageFormat storage_format,
        std::span<const glm::vec3> vertices
    );    

    MeshID createMesh(
        MeshStorageFormat storage_format,
        uint32_t vertex_count
    );

    void setMeshVertexData(
        MeshID mesh,
        std::span<const glm::vec3> vertices
    );

    MaterialID createMaterial(
        std::span<const std::byte> vert_shader_binary,
        std::span<const std::byte> frag_shader_binary
    );

    ModelID createModel(
        MeshID mesh,
        MaterialID material,
        const glm::mat4& trans = glm::mat4(1.0f)
    );

    void destroyModel(
        ModelID model
    );

    void getModelTransform(
        ModelID model
    ) const;

    void setModelTransform(
        ModelID model,
        const glm::mat4& trans
    );

    std::tuple<uint32_t, uint32_t> getViewport() const;
    void setViewport(uint32_t width, uint32_t height);

    void draw(Vulkan::ISwapchain* swapchain);

    Vulkan::Scene& vulkanAPI() {
        return reinterpret_cast<Vulkan::Scene&>(*this);
    }

    const Vulkan::Scene& vulkanAPI() const {
        return reinterpret_cast<const Vulkan::Scene&>(*this);
    }
};
}
