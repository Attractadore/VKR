#pragma once
#include "Scene.hpp"
#include "VKRVulkan.hpp"
#include "WSISwapchain.hpp"

namespace VKR {

class Device: public Vulkan::GraphicsDeviceConnection {
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    QueueFamilies m_queue_families;
    VkDevice m_device = VK_NULL_HANDLE;
    Queues m_queues;

    std::vector<SceneImpl> m_scenes;
    std::vector<WSISwapchain> m_wsi_swapchains;

public:
    Device(
        const PhysicalDevice& dev,
        const GraphicsDeviceConnectionFeatures& conf
    );

    ~Device() {
        destroy();
    }

    void create(
        const GraphicsDeviceConnectionFeatures& conf
    );

    void destroy();

    VkInstance getInstance() const {
        return m_instance;
    }

    VkPhysicalDevice getPhysicalDevice() const {
        return m_physical_device;
    }

    const QueueFamilies& getQueueFamilies() const {
        return m_queue_families;
    }

    VkDevice getDevice() const {
        return m_device;
    }

    const Queues& getQueues() const {
        return m_queues;
    }

    SceneImpl& createSceneImpl(
        const Camera& camera, uint32_t width, uint32_t height
    );

    bool WSISwapchainPresentModeSupported(
        VkSurfaceKHR surf, VkPresentModeKHR pmode
    ) const;

    WSISwapchain& createWSISwapchain(
        VkSurfaceKHR surf,
        VkExtent2D extent,
        VkPresentModeKHR pmode
    );
};

class PhysicalDevice: public Vulkan::GraphicsDevice {
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    QueueFamilies m_queue_families;

    std::vector<Device> m_devices;

public:
    PhysicalDevice(VkInstance instance, VkPhysicalDevice dev);

    VkInstance getInstance() const {
        return m_instance;
    }

    VkPhysicalDevice getPhysicalDevice() const {
        return m_physical_device;
    }

    const QueueFamilies& getQueueFamilies() const {
        return m_queue_families;
    }

    bool canUse() const {
        return m_queue_families.graphics != QueueFamilies::NotFound;
    }

    explicit operator bool() const {
        return canUse();
    }

    bool extensionSupported(const char* ext) const {
        std::array exts = {
            ext,
        };
        return extensionsSupported(exts);
    }

    bool extensionsSupported(std::span<const char* const> exts) const;

    std::string name() const;
    bool presentSupported() const;

    Device& createDevice(
        const GraphicsDeviceConnectionFeatures& conf
    );
};
}
