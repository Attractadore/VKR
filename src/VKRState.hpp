#pragma once

#include "VKRMeshInternal.hpp"

#include <memory>
#include <vector>

enum {
    VKR_NUM_INSTANCE_LAYERS_MAX = 16,
    VKR_NUM_INSTANCE_EXTENSIONS_MAX = 32,
    VKR_NUM_DEVICE_EXTENSIONS_MAX = 256,

    VKR_NUM_SURFACE_FORMATS_MAX = 16,
    VKR_NUM_SURFACE_PRESENT_MODES_MAX = 8,

    VKR_NUM_SWAPCHAIN_IMAGES_MAX = 8,
};

typedef struct {
    int graphics;
} QueueFamilies;

typedef struct {
    VkQueue graphics;
} Queues;

typedef struct {
    struct {
        VkInstance instance;
#ifndef NDEBUG
        VkDebugUtilsMessengerEXT debug_messenger;
#endif
    };

    VkSurfaceKHR surface;

    struct {
        VkPhysicalDevice physical_device;
        QueueFamilies queue_families;
        VkDevice device;
        Queues queues;
    };

    VkCommandPool command_pool;
    VkCommandPool copy_pool;

    VkRenderPass render_pass;

    struct {
        VkSwapchainKHR swapchain;
        VkSurfaceFormatKHR swapchain_format;
        VkPresentModeKHR swapchain_present_mode;
        VkExtent2D swapchain_extent;
        VkImageView swapchain_image_views[VKR_NUM_SWAPCHAIN_IMAGES_MAX];
        VkFramebuffer swapchain_framebuffers[VKR_NUM_SWAPCHAIN_IMAGES_MAX];
        VKRImage depth_buffers[VKR_NUM_SWAPCHAIN_IMAGES_MAX];
        VkImageView depth_buffer_views[VKR_NUM_SWAPCHAIN_IMAGES_MAX];
        VkFormat depth_buffer_format;
        unsigned num_swapchain_images;
    };

    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    struct {
        unsigned frame;
        VkCommandBuffer command_buffers[VKR_NUM_SWAPCHAIN_IMAGES_MAX];
        VkSemaphore acquire_semaphores[VKR_NUM_SWAPCHAIN_IMAGES_MAX];
        VkSemaphore draw_semaphores[VKR_NUM_SWAPCHAIN_IMAGES_MAX];
        VkFence frame_fences[VKR_NUM_SWAPCHAIN_IMAGES_MAX];
    };

    glm::mat4 proj_view;

    std::vector<std::unique_ptr<VKR::StaticMesh>> static_meshes;
    std::vector<std::unique_ptr<VKR::StreamingMesh>> streaming_meshes;

    std::vector<std::unique_ptr<VKR::Model>> models;
} VKRState;

extern VKRState vkr;
