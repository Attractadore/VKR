#pragma once
#include <vulkan/vulkan.h>

#include <algorithm>

#define MIN(a, b) std::min(a, b) 
#define MAX(a, b) std::max(a, b) 
#define CLAMP(x, low, high) std::clamp(x, low, high) 

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define BIT_SET(x, i) ((x) & (1 << (i)))
#define SET_BIT(x, i) do { (x) |= (1 << (i)); } while(false)

#define SWAP(t, l, r) std::swap<t>(l, r)

#define STRING(s) #s

#define STYPE(s) stype<decltype(s)>()

template<class VulkanStruct>
constexpr VkStructureType stype();

template<> constexpr VkStructureType stype<VkBufferCreateInfo>() { return VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;}
template<> constexpr VkStructureType stype<VkCommandBufferAllocateInfo>() { return VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;}
template<> constexpr VkStructureType stype<VkCommandBufferBeginInfo>() { return VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;}
template<> constexpr VkStructureType stype<VkCommandPoolCreateInfo>() { return VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;}
template<> constexpr VkStructureType stype<VkFenceCreateInfo>() { return VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;}
template<> constexpr VkStructureType stype<VkImageCreateInfo>() { return VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;}
template<> constexpr VkStructureType stype<VkMemoryAllocateInfo>() { return VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;}
template<> constexpr VkStructureType stype<VkPipelineDepthStencilStateCreateInfo>() { return VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;}
template<> constexpr VkStructureType stype<VkPresentInfoKHR>() { return VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;}
template<> constexpr VkStructureType stype<VkRenderPassBeginInfo>() { return VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;}
template<> constexpr VkStructureType stype<VkSemaphoreCreateInfo>() { return VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;}
template<> constexpr VkStructureType stype<VkSubmitInfo>() { return VK_STRUCTURE_TYPE_SUBMIT_INFO;}

typedef const char* (*AccessFunctionT)(const void*);

bool VKR_FeatureSupported(
    const void* supported,
    unsigned num_supported,
    unsigned supported_size,
    const char* feature,
    AccessFunctionT accessor
);
bool VKR_FeaturesSupported(
    const void* supported,
    unsigned num_supported,
    unsigned supported_size,
    const char** features,
    unsigned num_features,
    AccessFunctionT accessor
);
