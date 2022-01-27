#include "GraphicsDevice.hpp"

#include <algorithm>

namespace VKR {
namespace {
QueueFamilies findQueueFamilies(VkPhysicalDevice device) {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());

    uint32_t graphics_family = QueueFamilies::NotFound;
    for (uint32_t i = 0; i < count; i++) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family = i;
            break;
        }
    }

    return {
        .graphics = graphics_family,
    };
}

bool extensionSupported(const char* ext, std::span<const VkExtensionProperties> ext_props) {
    auto it = std::ranges::find(
        ext_props, ext,
        [](const VkExtensionProperties& ext_prop) { return ext_prop.extensionName; }
    );
    return it != ext_props.end();
}

VkDevice createDevice(
    VkPhysicalDevice physical_device, const QueueFamilies& queue_families,
    std::span<const char* const> extensions
) {
    assert(queue_families.graphics != QueueFamilies::NotFound);
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_families.graphics,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkDevice device;
    vkCreateDevice(physical_device, &device_create_info, nullptr, &device);

    return device;
}

Queues findQueues(VkDevice dev, const QueueFamilies& queue_families) {
    assert(queue_families.graphics != QueueFamilies::NotFound);
    Queues queues;
    vkGetDeviceQueue(dev, queue_families.graphics, 0, &queues.graphics);
    return queues;
}
}

PhysicalDevice::PhysicalDevice(
    VkInstance instance, VkPhysicalDevice dev
):
    m_instance(instance),
    m_physical_device(dev),
    m_queue_families(findQueueFamilies(m_physical_device)) {
}

bool PhysicalDevice::extensionsSupported(std::span<const char* const> exts) const {
    uint32_t ext_cnt;
    vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &ext_cnt, nullptr);
    std::vector<VkExtensionProperties> ext_props(ext_cnt);
    vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &ext_cnt, ext_props.data());

    return std::ranges::all_of(
        exts, [&](const char* ext) { return ::VKR::extensionSupported(ext, ext_props); }
    );
}

std::string PhysicalDevice::name() const {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(m_physical_device, &props);
    return props.deviceName;
}

bool PhysicalDevice::presentSupported() const {
    return extensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

Device& PhysicalDevice::createDevice(
    const GraphicsDeviceConnectionFeatures& conf
) {
    if (conf.present) {
        assert(presentSupported());
    }
    return m_devices.emplace_back(*this, conf);
}

std::string GraphicsDevice::name() const {
    return static_cast<const PhysicalDevice*>(this)->name();
}

bool GraphicsDevice::presentSupported() const {
    return static_cast<const PhysicalDevice*>(this)->presentSupported();
}

GraphicsDeviceConnection& GraphicsDevice::createConnection(
    const GraphicsDeviceConnectionFeatures& conf
) {
    return static_cast<PhysicalDevice*>(this)->createDevice(conf);
}

Device::Device(
    const PhysicalDevice& dev,
    const GraphicsDeviceConnectionFeatures& conf
): m_instance(dev.getInstance()),
   m_physical_device(dev.getPhysicalDevice()),
   m_queue_families(dev.getQueueFamilies()) {
    create(conf);
}

void Device::create(
    const GraphicsDeviceConnectionFeatures& conf
) {
    std::vector<const char*> exts;
    if (conf.present) {
        exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    m_device = createDevice(m_physical_device, m_queue_families, exts);
    m_queues = findQueues(m_device, m_queue_families); 
}

void Device::destroy() {
    if (m_device) {
        vkDeviceWaitIdle(m_device);
        vkDestroyDevice(m_device, nullptr);
    }
}

SceneImpl& Device::createSceneImpl(
    const Camera& camera, uint32_t width, uint32_t height
) {

    return m_scenes.emplace_back(
        camera,
        *this,
        width, height
    );
}

bool Device::WSISwapchainPresentModeSupported(
    VkSurfaceKHR surf, VkPresentModeKHR pmode
) const {
    return WSIPresentModeSupported(m_physical_device, surf, pmode);
}

WSISwapchain& Device::createWSISwapchain(
    VkSurfaceKHR surf, VkExtent2D ext, VkPresentModeKHR pmode
) {
    // TODO: maybe do something to prevent swapchain creation for the
    // same surface
    auto& swp = m_wsi_swapchains.emplace_back(
        m_physical_device, m_queue_families.graphics,
        m_device, m_queues.graphics,
        surf
    );
    swp.create(ext, pmode);
    return swp;
}

Scene& GraphicsDeviceConnection::createScene(
    const Camera& camera, uint32_t width, uint32_t height
) {
    return static_cast<Device*>(this)->createSceneImpl(camera, width, height);
}

bool Vulkan::GraphicsDeviceConnection::WSISwapchainPresentModeSupported(
    VkSurfaceKHR surf,
    VkPresentModeKHR pmode
) const {
    return static_cast<const Device*>(this)->WSISwapchainPresentModeSupported(
        surf, pmode
    );
}

Vulkan::IWSISwapchain& Vulkan::GraphicsDeviceConnection::createWSISwapchain(
    VkSurfaceKHR surf,
    VkExtent2D extent,
    VkPresentModeKHR pmode
) {
    return static_cast<Device*>(this)->createWSISwapchain(
        surf, extent, pmode
    );
}
}
