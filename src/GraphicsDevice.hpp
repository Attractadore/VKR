#pragma once
#include "Scene.hpp"

#include <memory>

namespace VKR {
class PhysicalDevice;
class Device;

namespace Detail {
struct DeviceHandleDeleter {
    void operator()(VkDevice device) {
        vkDeviceWaitIdle(device);
        vkDestroyDevice(device, nullptr);
    }
};

using VkDeviceUniqueHandle = std::unique_ptr<
    std::remove_pointer_t<VkDevice>, DeviceHandleDeleter
>;
}

class Device: public Vulkan::GraphicsDeviceConnection {
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    QueueFamilies m_queue_families;
    Detail::VkDeviceUniqueHandle m_device = VK_NULL_HANDLE;
    Queues m_queues;

    std::vector<SceneImpl> m_scenes;

public:
    Device(
        const PhysicalDevice& dev,
        const GraphicsDeviceConnectionFeatures& conf
    );

    void create(
        const GraphicsDeviceConnectionFeatures& conf
    );

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
        return m_device.get();
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
};

class PhysicalDevice: public Vulkan::GraphicsDevice {
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
    QueueFamilies m_queue_families;

    VkPhysicalDeviceProperties m_properties;

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

    const char* name() const {
        return m_properties.deviceName;
    }

    bool presentSupported() const;

    Device& createDevice(
        const GraphicsDeviceConnectionFeatures& conf
    );
};
}
