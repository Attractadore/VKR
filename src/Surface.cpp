#include "Surface.hpp"
#include "GraphicsDevice.hpp"

namespace VKR {
void Surface::create(VkInstance instance, Vulkan::VkSurfaceKHRCreator create_surface) {
    VkSurfaceKHR surf;
    create_surface(instance, nullptr, &surf);
    m_surface.reset(surf);
}

bool Surface::presentModeSupported(
    const Device* dev,
    VkPresentModeKHR pmode
) const {
    return ::VKR::presentModeSupported(dev->getPhysicalDevice(), m_surface.get(), pmode);
}

Swapchain& Surface::createSwapchain(
    Device* dev,
    VkExtent2D ext,
    VkPresentModeKHR pmode
) {
    m_swapchain = std::make_unique<Swapchain>(
        dev->getPhysicalDevice(), dev->getQueueFamilies().graphics,
        dev->getDevice(), dev->getQueues().graphics,
        m_surface.get()
    );
    m_swapchain->create(ext, pmode);
    return *m_swapchain;
}

bool Vulkan::WSISurface::presentModeSupported(
    const VKR::GraphicsDeviceConnection* connection,
    VkPresentModeKHR pmode
) const {
    return static_cast<const Surface*>(this)->presentModeSupported(
        static_cast<const Device*>(connection), pmode
    );
}

Vulkan::IWSISwapchain& Vulkan::WSISurface::createWSISwapchain(
    VKR::GraphicsDeviceConnection* connection,
    VkExtent2D extent,
    VkPresentModeKHR pmode
) {
    return static_cast<Surface*>(this)->createSwapchain(
        static_cast<Device*>(connection), extent, pmode
    );
}
}
