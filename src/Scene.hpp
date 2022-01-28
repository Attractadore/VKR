#pragma once
#include "Image.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Queues.hpp"
#include "VKRVulkan.hpp"

#include <stack>
#include <vector>

namespace VKR {
class Device;

class SceneImpl: public VKR::Scene {
    VkInstance m_instance;

    VkPhysicalDevice m_physical_device;
    QueueFamilies m_queue_families;    

    VkDevice m_device;
    Queues m_queues;

    VmaAllocator m_allocator;

    static constexpr size_t c_img_cnt = 3;
    size_t m_cur_img = 0;
    uint32_t m_width;
    uint32_t m_height;
    std::array<Image, c_img_cnt> m_color_imgs;
    std::array<VkImageView, c_img_cnt> m_color_views = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
    };
    Image m_depth_img;
    VkImageView m_depth_view = VK_NULL_HANDLE;

    VkRenderPass m_render_pass = VK_NULL_HANDLE;

    std::array<VkFramebuffer, c_img_cnt> m_fbs = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
    };

    VkCommandPool m_cmd_pool;
    VkCommandPool m_transient_cmd_pool;

    std::vector<StaticMesh> m_static_meshes;
    std::vector<DynamicMesh> m_dynamic_meshes;

    std::vector<Material> m_mats;

    std::vector<StaticModel> m_static_models;
    std::vector<DynamicModel> m_dynamic_models;

    std::stack<Detail::modelid> m_static_model_id_pool;
    std::stack<Detail::modelid> m_dynamic_model_id_pool;

    std::array<VkCommandBuffer, c_img_cnt> m_cmd_bufs = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
    };

    std::array<VkSemaphore, c_img_cnt> m_dst_sems = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
    };
    std::array<VkFence, c_img_cnt> m_fences = {
        VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE,
    };

    float m_near = 0.1;
    float m_far = 100.0f;

public:
    SceneImpl(
        const Camera& cam,
        const Device& dev,
        uint32_t width, uint32_t height
    );
    SceneImpl(const Scene& other) = delete;
    SceneImpl(Scene&& other);
    SceneImpl& operator=(const Scene& other) = delete;
    SceneImpl& operator=(Scene&& other);

    ~SceneImpl() {
        destroy();
    }

    void create();
    void destroy();

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
        const glm::mat4& t
    );

    void destroyModel(ModelID model);

    void setModelTransform(
        ModelID model,
        const glm::mat4& t
    );

    std::tuple<uint32_t, uint32_t> getViewport() const;
    void setViewport(uint32_t width, uint32_t height);

    void draw(Vulkan::ISwapchain* swapchain);

private:
    // TODO: enum-based polymorphism sucks
    StaticMesh& getStaticMesh(MeshID mesh);
    std::tuple<MeshID, StaticMesh*> getNewStaticMesh();
    MeshID createStaticMesh(std::span<const glm::vec3> vertices);

    DynamicMesh& getDynamicMesh(MeshID mesh);
    std::tuple<MeshID, DynamicMesh*> getNewDynamicMesh();
    MeshID createDynamicMesh(uint32_t vertex_count);
    void setDynamicMeshVertexData(MeshID id, std::span<const glm::vec3> vertices);

    Material& getMaterial(MaterialID material);

    StaticModel& getStaticModel(ModelID model);
    std::tuple<ModelID, StaticModel*> getNewStaticModel();
    void returnStaticModelIDToPool(Detail::modelid id);
    ModelID createStaticModel(
        MeshID mesh,
        MaterialID material, const glm::mat4& t
    );
    void destroyStaticModel(ModelID id);
    void setStaticModelTransform(
        ModelID id,
        const glm::mat4& t
    );

    DynamicModel& getDynamicModel(ModelID model);
    std::tuple<ModelID, DynamicModel*> getNewDynamicModel();
    void returnDynamicModelIDToPool(Detail::modelid id);
    ModelID createDynamicModel(
        MeshID mesh,
        MaterialID material, const glm::mat4& t
    );
    void destroyDynamicModel(ModelID id);
    void setDynamicModelTransform(
        ModelID id,
        const glm::mat4& t
    );

    VkSemaphore draw(
        const glm::mat4& proj_view,
        VkImage dst_img,
        uint32_t dst_img_width, uint32_t dst_img_height,
        VkSemaphore dst_img_sem,
        Vulkan::LayoutTransitionToTransferDstInserter to_ins,
        Vulkan::LayoutTransitionFromTransferDstInserter from_ins
    );

    glm::mat4 getProj() const;
    glm::mat4 getView() const;
};
}
