#pragma once
#include <vulkan/vulkan.h>

namespace VKR {
struct QueueFamilies {
    static constexpr uint32_t NotFound = -1;
    uint32_t graphics = NotFound;
};

struct Queues {
    VkQueue graphics = VK_NULL_HANDLE;
};
}
