#include "GraphicsDevice.hpp"
#include "IDPacking.hpp"
#include "Internal.hpp"
#include "Scene.hpp"
#include "Sync.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace VKR {
namespace {
VmaAllocator createAllocator(
    VkInstance instance,
    VkPhysicalDevice physical_device, VkDevice device
) {
    VmaAllocatorCreateInfo create_info = {
        .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
        .physicalDevice = physical_device,
        .device = device,
        .instance = instance,
    };

    VmaAllocator allocator;
    vmaCreateAllocator(&create_info, &allocator);
    return allocator;
}

bool formatSupported(
    VkPhysicalDevice device,
    VkFormat format, VkFormatFeatureFlags flags
) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(device, format, &props);
    VkFormatFeatureFlags supported_flags = props.optimalTilingFeatures;
    return (supported_flags & flags) == flags;
}

VkFormat selectFormat(
    VkPhysicalDevice device,
    std::span<const VkFormat> formats, VkFormatFeatureFlags flags
) {
    for (const auto& f: formats) {
        if (formatSupported(device, f, flags)) {
            return f;
        }
    }
    return VK_FORMAT_UNDEFINED;
}

VkFormat selectColorFormat(
    VkPhysicalDevice device,
    std::span<const VkFormat> formats
) {
    return selectFormat(
        device, formats,
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT
    );
}

VkFormat selectDepthFormat(
    VkPhysicalDevice device,
    std::span<const VkFormat> formats
) {
    return selectFormat(
        device, formats,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkRenderPass createRenderPass(
    VkDevice device,
    VkFormat color_format, VkFormat depth_format
) {
    VkAttachmentDescription color_attachment = {
        .format = color_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    };

    VkAttachmentDescription depth_attachment = {
        .format = depth_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    std::array attachments = {
        color_attachment,
        depth_attachment,
    };

    VkAttachmentReference color_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_reference = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_reference,
        .pDepthStencilAttachment = &depth_reference,
    };

    VkSubpassDependency in_dep_color = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkSubpassDependency in_dep_depth = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    VkSubpassDependency out_dep_color = {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
    };

    std::array deps = {
        in_dep_color, in_dep_depth, out_dep_color
    };

    VkRenderPassCreateInfo render_pass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = deps.size(),
        .pDependencies = deps.data(),
    };
    
    VkRenderPass render_pass;
    vkCreateRenderPass(device, &render_pass_create_info, nullptr, &render_pass);

    return render_pass;
}

VkFramebuffer createFramebuffer(
    VkDevice device,
    VkRenderPass render_pass,
    VkImageView color_view, VkImageView depth_view,
    uint32_t width, uint32_t height
) {
    std::array attachments = {
        color_view, depth_view,
    };
    VkFramebufferCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = render_pass,
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .width = width,
        .height = height,
        .layers = 1,
    };

    VkFramebuffer framebuffer;
    vkCreateFramebuffer(device, &create_info, nullptr, &framebuffer);
    return framebuffer;
}

VkCommandPool createCommandPool(
    VkDevice device,
    VkCommandPoolCreateFlags flags,
    uint32_t queue_family
) {
    VkCommandPoolCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = flags,
        .queueFamilyIndex = queue_family,
    };

    VkCommandPool command_pool;
    vkCreateCommandPool(device, &create_info, nullptr, &command_pool);
    return command_pool;
}

auto createMainCommandPool(
    VkDevice device,
    uint32_t queue_family
) {
    return createCommandPool(device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, queue_family);
}

auto createTransientCommandPool(
    VkDevice device,
    uint32_t queue_family
) {
    return createCommandPool(device, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, queue_family);
}

void allocateCommandBuffers(
    VkDevice device,
    VkCommandPool cmd_pool,
    std::span<VkCommandBuffer> cmd_buffers
) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(cmd_buffers.size()),
    };
    vkAllocateCommandBuffers(device, &alloc_info, cmd_buffers.data());
}
}

SceneImpl::SceneImpl(
    const Camera& cam,
    const Device& dev,
    uint32_t width, uint32_t height
):  m_instance(dev.getInstance()),
    m_physical_device(dev.getPhysicalDevice()),
    m_queue_families(dev.getQueueFamilies()),
    m_device(dev.getDevice()),
    m_queues(dev.getQueues()),
    m_width(width), 
    m_height(height)
{   
    m_camera = cam;
    create();
}

void SceneImpl::create() {
    m_allocator = createAllocator(
        m_instance,
        m_physical_device, m_device
    );

    auto color_fmt =
        selectColorFormat(m_physical_device, color_fmts);
    assert(color_fmt != VK_FORMAT_UNDEFINED);
    auto depth_fmt =
        selectDepthFormat(m_physical_device, depth_fmts);
    assert(depth_fmt != VK_FORMAT_UNDEFINED);

    for (size_t i = 0; i < c_img_cnt; i++) {
        m_color_imgs[i] =
            createColorImage(m_allocator, color_fmt, m_width, m_height);
        m_color_views[i] =
            createColorImageView(m_device, m_color_imgs[i].image, color_fmt);
    }
    m_depth_img =
        createDepthImage(m_allocator, depth_fmt, m_width, m_height);
    m_depth_view =
        createDepthImageView(m_device, m_depth_img.image, depth_fmt);

    m_render_pass = createRenderPass(m_device, color_fmt, depth_fmt);

    for (size_t i = 0; i < c_img_cnt; i++) {
        m_fbs[i] = createFramebuffer(
            m_device,
            m_render_pass,
            m_color_views[i], m_depth_view,
            m_width, m_height
        );
    }

    m_cmd_pool =
        createMainCommandPool(m_device, m_queue_families.graphics);
    m_transient_cmd_pool =
        createTransientCommandPool(m_device, m_queue_families.graphics);

    allocateCommandBuffers(m_device, m_cmd_pool, m_cmd_bufs);
    
    for (auto& sem: m_dst_sems) {
        sem = createSemaphore(m_device);
    }
    for (auto& fence: m_fences) {
        fence = createSignaledFence(m_device);
    }
}

void SceneImpl::destroy() {
    if (m_device) {
        vkDeviceWaitIdle(m_device);

        for (auto& model: m_static_models) {
            model.destroy();
        }
        m_static_models.clear();
        for (auto& model: m_dynamic_models) {
            model.destroy();
        }
        m_dynamic_models.clear();

        for (auto& mat: m_mats) {
            mat.destroy(m_device);
        }
        m_mats.clear();

        for (auto& mesh: m_static_meshes) {
            mesh.destroy(m_allocator);
        }
        m_static_meshes.clear();
        for (auto& mesh: m_dynamic_meshes) {
            mesh.destroy(m_allocator);
        }
        m_dynamic_meshes.clear();

        for (auto& fence: m_fences) {
            vkDestroyFence(m_device, fence, nullptr);
        }
        for (auto& sem: m_dst_sems) {
            vkDestroySemaphore(m_device, sem, nullptr);
        }

        vkFreeCommandBuffers(m_device, m_cmd_pool, m_cmd_bufs.size(), m_cmd_bufs.data());

        vkDestroyCommandPool(m_device, m_transient_cmd_pool, nullptr);
        vkDestroyCommandPool(m_device, m_cmd_pool, nullptr);

        for (auto& fb: m_fbs) {
            vkDestroyFramebuffer(m_device, fb, nullptr);
        }

        vkDestroyRenderPass(m_device, m_render_pass, nullptr);

        if (m_allocator) {
            for (auto& v: m_color_views) {
                vkDestroyImageView(m_device, v, nullptr);
            }
            for (auto& i: m_color_imgs) {
                i.destroy(m_allocator);
            }
            vkDestroyImageView(m_device, m_depth_view, nullptr);
            m_depth_img.destroy(m_allocator);

            vmaDestroyAllocator(m_allocator);
        }
    }
}

MeshID SceneImpl::createMesh(
    MeshStorageFormat storage_format,
    std::span<const glm::vec3> vertices
) {
    // TODO: maybe allow dynamic meshes
    assert(storage_format == MeshStorageFormat::Static);
    return createStaticMesh(vertices);
}

MeshID SceneImpl::createMesh(
    MeshStorageFormat storage_format,
    uint32_t vertex_count 
) {
    // TODO: maybe allow static meshes
    assert(storage_format == MeshStorageFormat::Dynamic);
    return createDynamicMesh(vertex_count);
}

void SceneImpl::setMeshVertexData(
    MeshID mesh,
    std::span<const glm::vec3> vertices
) {
    // TODO: maybe allow static meshes
    setDynamicMeshVertexData(mesh, vertices);
}

MaterialID SceneImpl::createMaterial(
    std::span<const std::byte> vert_shader_binary,
    std::span<const std::byte> frag_shader_binary
) {
    auto id = static_cast<MaterialID>(
        m_mats.size()
    );
    auto& material = m_mats.emplace_back();
    material.create(
        m_device,
        vert_shader_binary, frag_shader_binary,
        m_render_pass
    );

    return id;

}

ModelID SceneImpl::createModel(
    MeshID mesh,
    MaterialID material,
    const glm::mat4& t
) {
    using enum MeshStorageFormat;
    switch (getMeshStorageFormat(mesh)) {
        case Static:
            return createStaticModel(mesh, material, t);
        case Dynamic:
            return createDynamicModel(mesh, material, t);
    }
}

void SceneImpl::destroyModel(ModelID model) {
    using enum MeshStorageFormat;
    switch (getModelMeshStorageFormat(model)) {
        case Static:
            return destroyStaticModel(model);
        case Dynamic:
            return destroyDynamicModel(model);
    }
}

void SceneImpl::setModelTransform(
    ModelID model,
    const glm::mat4& t
) {
    using enum MeshStorageFormat;
    switch (getModelMeshStorageFormat(model)) {
        case Static:
            return setStaticModelTransform(model, t);
        case Dynamic:
            return setDynamicModelTransform(model, t);
    }
    assert(!"Invalid enum value");
}

std::tuple<uint32_t, uint32_t> SceneImpl::getViewport() const {
    return {m_width, m_height};
}

void SceneImpl::setViewport(uint32_t width, uint32_t height) {
    assert(!"Not implemented");
}

void SceneImpl::draw(Vulkan::ISwapchain* swapchain) {
    auto proj_view = getProj() * getView();
    auto [img_idx, img_sem, fence] = swapchain->acquireImage();
    auto img = swapchain->getImage(img_idx);
    auto ext = swapchain->getExtent();
    auto draw_sem = draw(
        proj_view,
        img, ext.width, ext.height,
        img_sem,
        swapchain->getLayoutTransitionToTransferDstInserter(),
        swapchain->getLayoutTransitionFromTransferDstInserter()
    );
    swapchain->presentImage(img_idx, draw_sem);
}

glm::mat4 SceneImpl::getProj() const {
    auto proj = glm::perspectiveRH_ZO(
        m_camera.m_vfov, m_camera.m_aspect_ratio,
        m_near, m_far
    );
    proj[1][1] = -proj[1][1];
    return proj;
}

StaticMesh& SceneImpl::getStaticMesh(MeshID mesh) {
    assert(getMeshStorageFormat(mesh) == MeshStorageFormat::Static);
    auto i = getMeshIndex(mesh);;
    return m_static_meshes[i];
}

std::tuple<MeshID, StaticMesh*> SceneImpl::getNewStaticMesh() {
    auto id = makeMeshID(m_static_meshes.size(), MeshStorageFormat::Static);
    auto meshp = &m_static_meshes.emplace_back();
    return {id, meshp};
}

MeshID SceneImpl::createStaticMesh(std::span<const glm::vec3> vertices) {
    auto [id, mesh] = getNewStaticMesh();
    mesh->create(
        m_device, m_allocator,
        m_queues.graphics, m_transient_cmd_pool,
        vertices
    );

    return id;
}

DynamicMesh& SceneImpl::getDynamicMesh(MeshID mesh) {
    assert(getMeshStorageFormat(mesh) == MeshStorageFormat::Dynamic);
    auto i = getMeshIndex(mesh);
    return m_dynamic_meshes[i];
}

std::tuple<MeshID, DynamicMesh*> SceneImpl::getNewDynamicMesh() {
    auto id = makeMeshID(m_dynamic_meshes.size(), MeshStorageFormat::Dynamic);
    auto meshp = &m_dynamic_meshes.emplace_back();
    return {id, meshp};
}

MeshID SceneImpl::createDynamicMesh(uint32_t vertex_count) {
    auto [id, mesh] = getNewDynamicMesh();
    mesh->create(
        m_device, m_allocator,
        m_queues.graphics,
        vertex_count
    );

    return id;
}

void SceneImpl::setDynamicMeshVertexData(MeshID id, std::span<const glm::vec3> vertices) {
    auto& mesh = getDynamicMesh(id);
    mesh.setVertexData(m_device, m_allocator, vertices);
}

Material& SceneImpl::getMaterial(MaterialID material) {
    auto i = static_cast<size_t>(material);
    return m_mats[i];
}

StaticModel& SceneImpl::getStaticModel(ModelID model) {
    assert(getModelMeshStorageFormat(model) == MeshStorageFormat::Static);
    auto i = getModelIndex(model);
    return m_static_models[i];
}

std::tuple<ModelID, StaticModel*> SceneImpl::getNewStaticModel() {
    if (m_static_model_id_pool.empty()) {
        auto id = makeModelID(m_static_models.size(), MeshStorageFormat::Static);
        auto modelp = &m_static_models.emplace_back();
        return {id, modelp};
    } else {
        auto index = m_static_model_id_pool.top();
        m_static_model_id_pool.pop();
        auto id = makeModelID(index, MeshStorageFormat::Static);
        auto modelp = &m_static_models[index];
        return {id, modelp};
    }
}

void SceneImpl::returnStaticModelIDToPool(Detail::modelid id) {
    m_static_model_id_pool.push(id);
}

ModelID SceneImpl::createStaticModel(
    MeshID mesh,
    MaterialID material, const glm::mat4& t
) {
    auto [id, model] = getNewStaticModel();
    model->create(mesh, material, t);
    return id;
}

void SceneImpl::destroyStaticModel(ModelID id) {
    auto& model = getStaticModel(id);
    model.destroy();
    returnStaticModelIDToPool(id);
}

void SceneImpl::setStaticModelTransform(
    ModelID id,
    const glm::mat4& t
) {
    auto& model = getStaticModel(id);
    model.transform = t;
}

DynamicModel& SceneImpl::getDynamicModel(ModelID model) {
    assert(getModelMeshStorageFormat(model) == MeshStorageFormat::Dynamic);
    auto i = getModelIndex(model);
    return m_dynamic_models[i];
}

std::tuple<ModelID, DynamicModel*> SceneImpl::getNewDynamicModel() {
    if (m_dynamic_model_id_pool.empty()) {
        auto id = makeModelID(m_dynamic_models.size(), MeshStorageFormat::Dynamic);
        auto modelp = &m_dynamic_models.emplace_back();
        return {id, modelp};
    } else {
        auto index = m_dynamic_model_id_pool.top();
        m_dynamic_model_id_pool.pop();
        auto id = makeModelID(index, MeshStorageFormat::Dynamic);
        auto modelp = &m_dynamic_models[index];
        return {id, modelp};
    }
}

void SceneImpl::returnDynamicModelIDToPool(Detail::modelid id) {
    m_dynamic_model_id_pool.push(id);
}

ModelID SceneImpl::createDynamicModel(
    MeshID mesh,
    MaterialID material, const glm::mat4& t
) {
    auto [id, model] = getNewDynamicModel();
    model->create(mesh, material, t);
    return id;
}

void SceneImpl::destroyDynamicModel(ModelID id) {
    auto& model = getDynamicModel(id);
    model.destroy();
    returnDynamicModelIDToPool(id);
}

void SceneImpl::setDynamicModelTransform(
    ModelID id,
    const glm::mat4& t
) {
    auto& model = getDynamicModel(id);
    model.transform = t;
}

VkSemaphore SceneImpl::draw(
    const glm::mat4& proj_view,
    VkImage dst_img,
    uint32_t dst_img_width, uint32_t dst_img_height,
    VkSemaphore dst_img_sem,
    Vulkan::LayoutTransitionToTransferDstInserter to_ins,
    Vulkan::LayoutTransitionFromTransferDstInserter from_ins
) {
        VkFence fence = m_fences[m_cur_img];
        vkWaitForFences(m_device, 1, &fence, true, UINT64_MAX);
        vkResetFences(m_device, 1, &fence);

        VkCommandBuffer cmd_buffer = m_cmd_bufs[m_cur_img];
        {
            VkCommandBufferBeginInfo begin_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            };
            vkBeginCommandBuffer(cmd_buffer, &begin_info);
        }

        {
            VkViewport viewport = {
                .width = static_cast<float>(m_width),
                .height = static_cast<float>(m_height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };
            VkRect2D scissor = {
                .extent = {
                    .width = m_width,
                    .height = m_height,
                },
            };
            vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
            vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
        }

        {
            VkClearValue clear_color = {
                .color = {
                    .float32 = {
                        0.0f, 0.0f, 0.0f, 1.0f,
                    },
                },
            };
            VkClearValue clear_depth = {
                .depthStencil = {
                    .depth = 1.0f,
                },
            };
            std::array clear_values = {
                clear_color,
                clear_depth,  
            };
            VkRenderPassBeginInfo begin_info = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = m_render_pass,
                .framebuffer = m_fbs[m_cur_img],
                .renderArea = {
                    .extent = {
                        .width = m_width,
                        .height = m_height,
                    },
                },
                .clearValueCount = clear_values.size(),
                .pClearValues = clear_values.data(),
            };
            vkCmdBeginRenderPass(cmd_buffer, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
        }

        // TODO: deduplicate this
        for (const auto& model: m_static_models) {
            auto& material = getMaterial(model.material);
            auto& mesh = getStaticMesh(model.mesh);

            glm::mat4 mvp = proj_view * model.transform;

            // TODO: reorder bind calls for greater efficiency
            // TODO: check for empty model slots
            material.bind(cmd_buffer);
            material.setMVP(cmd_buffer, mvp);
            mesh.bind(cmd_buffer);
            mesh.draw(cmd_buffer);
        }

        for (const auto& model: m_dynamic_models) {
            auto& material = getMaterial(model.material);
            auto& mesh = getDynamicMesh(model.mesh);

            glm::mat4 mvp = proj_view * model.transform;

            // TODO: reorder bind calls for greater efficiency
            // TODO: check for empty model slots
            material.bind(cmd_buffer);
            material.setMVP(cmd_buffer, mvp);
            mesh.bind(cmd_buffer);
            mesh.draw(cmd_buffer);
        }

        vkCmdEndRenderPass(cmd_buffer);

        {
            VkImageMemoryBarrier to_layout_bar = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .dstQueueFamilyIndex = m_queue_families.graphics,
                .image = dst_img,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };
            to_ins(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, to_layout_bar);

            VkImageBlit region = {
                .srcSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .srcOffsets = {
                    {},
                    {
                        .x = static_cast<int32_t>(m_width),
                        .y = static_cast<int32_t>(m_height),
                        .z = 1
                    },
                },
                .dstSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .dstOffsets = {
                    {},
                    {
                        .x = static_cast<int32_t>(dst_img_width),
                        .y = static_cast<int32_t>(dst_img_height),
                        .z = 1
                    },
                },
            };

            // TODO: blits to (swapchain) images aren't supported on all platforms
            vkCmdBlitImage(cmd_buffer,
                m_color_imgs[m_cur_img].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                dst_img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &region,
                VK_FILTER_LINEAR
            );

            VkImageMemoryBarrier from_layout_bar = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = m_queue_families.graphics,
                .image = dst_img,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            };
            from_ins(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, from_layout_bar);
        }

        vkEndCommandBuffer(cmd_buffer);

        VkPipelineStageFlags transfer_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSemaphore dst_sem = m_dst_sems[m_cur_img];
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &dst_img_sem,
            .pWaitDstStageMask = &transfer_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &dst_sem,
        };

        vkQueueSubmit(m_queues.graphics, 1, &submit_info, fence);

        for (auto& mesh: m_dynamic_meshes) {
            mesh.updateFrameFrence(fence);
        }

        m_cur_img = (m_cur_img + 1) % c_img_cnt;

        return dst_sem;
    }

glm::mat4 SceneImpl::getView() const {
    return glm::lookAt(
        m_camera.m_position,
        m_camera.m_position + m_camera.m_forward,
        m_camera.m_up
    );
}

MeshID Scene::createMesh(
    MeshStorageFormat storage_format,
    std::span<const glm::vec3> vertices
) {
    return static_cast<SceneImpl*>(this)->createMesh(
        storage_format, vertices
    );
}

MeshID Scene::createMesh(
    MeshStorageFormat storage_format,
    uint32_t vertex_count
) {
    return static_cast<SceneImpl*>(this)->createMesh(
        storage_format, vertex_count 
    );
}

void Scene::setMeshVertexData(
    MeshID mesh,
    std::span<const glm::vec3> vertices
) {
    static_cast<SceneImpl*>(this)->setMeshVertexData(mesh, vertices);
}

MaterialID Scene::createMaterial(
    std::span<const std::byte> vert_shader_binary,
    std::span<const std::byte> frag_shader_binary
) {
    return static_cast<SceneImpl*>(this)->createMaterial(
        vert_shader_binary,
        frag_shader_binary
    );
}

ModelID Scene::createModel(
    MeshID mesh,
    MaterialID material,
    const glm::mat4& trans
) {
    return static_cast<SceneImpl*>(this)->createModel(
        mesh, material, trans
    );
}

void Scene::destroyModel(
    ModelID model
) {
    static_cast<SceneImpl*>(this)->destroyModel(model);
}

void Scene::getModelTransform(
    ModelID model
) const {
    static_cast<const SceneImpl*>(this)->getModelTransform(model);
}

void Scene::setModelTransform(
    ModelID model,
    const glm::mat4& trans
) {
    static_cast<SceneImpl*>(this)->setModelTransform(model, trans);
}

std::tuple<uint32_t, uint32_t> Scene::getViewport() const {
    return static_cast<const SceneImpl*>(this)->getViewport();
}
void Scene::setViewport(uint32_t width, uint32_t height) {
    static_cast<SceneImpl*>(this)->setViewport(width, height);
}

void Scene::draw(Vulkan::ISwapchain* swapchain) {
    static_cast<SceneImpl*>(this)->draw(swapchain);
}
}
