#pragma once
#include "Swapchain.hpp"
#include "VKRVulkan.hpp"

namespace VKR {
class Device;

namespace Detail {
class VkSurfaceKHRDeleter {
    VkInstance m_instance;
public:
    VkSurfaceKHRDeleter(VkInstance instance): m_instance(instance) {}

    void operator()(VkSurfaceKHR surf) {
        vkDestroySurfaceKHR(m_instance, surf, nullptr);
    }
};

using VkSurfaceKHRUniqueHandle = std::unique_ptr<
    std::remove_pointer_t<VkSurfaceKHR>, VkSurfaceKHRDeleter
>;
};

class Surface: public Vulkan::WSISurface {
    Detail::VkSurfaceKHRUniqueHandle m_surface;
    std::unique_ptr<Swapchain> m_swapchain = VK_NULL_HANDLE;

public:
    Surface(
        VkInstance instance,
        Vulkan::VkSurfaceKHRCreator create_surface
    ):
        m_surface(VK_NULL_HANDLE, Detail::VkSurfaceKHRDeleter(instance))
    {
        create(instance, create_surface);
    }

    bool presentModeSupported(
        const Device* dev,
        VkPresentModeKHR pmode
    ) const;

    Swapchain& createSwapchain(
        Device* dev,
        VkExtent2D extent,
        VkPresentModeKHR pmode
    );

private:
    void create(VkInstance instance, Vulkan::VkSurfaceKHRCreator create_surface);
};
}
