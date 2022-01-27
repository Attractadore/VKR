#include "Internal.hpp"
#include "VKR.hpp"
#include "GraphicsDevice.hpp"
#include "Instance.hpp"

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
    std::vector<VkPhysicalDevice> devices;
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

struct Instance::Impl {
    VkInstance instance = VK_NULL_HANDLE;
    std::vector<PhysicalDevice> devices;

    void create(std::span<const char* const> extensions) {
        instance = createInstance(extensions);

        devices = getPhysicalDevices(instance);
        // TODO: move filtering into getGraphicsDevices
        auto [b, e] = std::ranges::remove_if(
            devices,
            [] (const PhysicalDevice& dev) {
                return not dev;
            }
        );
        devices.erase(b, e);
    }

    void destroy() {
        vkDestroyInstance(instance, nullptr);
    }
};

Instance::Instance(std::span<const char* const> extensions):
    pimpl(std::make_unique<Impl>()) {
    pimpl->create(extensions);
}

Instance::~Instance() {
    pimpl->destroy();
}

uint32_t Instance::getGraphicsDeviceCount() const {
    return pimpl->devices.size();
}

const GraphicsDevice& Instance::getGraphicsDevice(uint32_t dev_idx) const {
    return pimpl->devices[dev_idx];
}
}
