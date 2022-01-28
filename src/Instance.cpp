#include "Internal.hpp"
#include "VKR.hpp"
#include "GraphicsDevice.hpp"
#include "Instance.hpp"
#include "Surface.hpp"

#include <algorithm>
#include <ranges>

namespace VKR {
namespace {
VkInstance createInstance(std::span<const char* const> extensions) {
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
    VkInstance instance;
    vkCreateInstance(&create_info, nullptr, &instance);
    return instance;
}

std::vector<PhysicalDevice> getPhysicalDevices(VkInstance instance) {
    uint32_t count;
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, devices.data()); 

    auto v = std::views::transform(
        devices,
        [&](const VkPhysicalDevice& dev) {
            return PhysicalDevice(instance, dev);
        }
    );

    return {v.begin(), v.end()};
}
}

namespace Detail {
struct VkInstanceDeleter {
    void operator()(VkInstance device) {
        vkDestroyInstance(device, nullptr);
    }
};

using VkInstanceUniqueHandle = std::unique_ptr<
    std::remove_pointer_t<VkInstance>, VkInstanceDeleter
>;
};

struct Instance::Impl {
    Detail::VkInstanceUniqueHandle instance = VK_NULL_HANDLE;
    std::vector<PhysicalDevice> devices;
    std::vector<Surface> m_surfaces;

    void create(std::span<const char* const> extensions) {
        instance.reset(createInstance(extensions));

        devices = getPhysicalDevices(instance.get());
        // TODO: move filtering into getGraphicsDevices
        auto [b, e] = std::ranges::remove_if(
            devices,
            [] (const PhysicalDevice& dev) {
                return not dev;
            }
        );
        devices.erase(b, e);
    }

    Surface& createSurface(Vulkan::VkSurfaceKHRCreator create_surface) {
        return m_surfaces.emplace_back(instance.get(), create_surface);
    }
};

Instance::Instance(const InstanceCreationFeatures& conf):
    pimpl(std::make_unique<Impl>())
{
    pimpl->create(conf.wsi_extensions);
}

Instance::~Instance() = default;

uint32_t Instance::getGraphicsDeviceCount() const {
    return pimpl->devices.size();
}

GraphicsDevice& Instance::getGraphicsDevice(uint32_t dev_idx) {
    return pimpl->devices[dev_idx];
}

Vulkan::WSISurface& Vulkan::Instance::createWSISurface(
    Vulkan::VkSurfaceKHRCreator create_surface
) {
    return pimpl->createSurface(create_surface);
}
}
