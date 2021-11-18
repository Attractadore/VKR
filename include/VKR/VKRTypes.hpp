#pragma once
#include <glm/vec3.hpp>

#include <vulkan/vulkan.h>

typedef VkResult (*VKR_GetInstanceExtensionsCallbackT)(unsigned*, const char**);
typedef VkResult (*VKR_CreateSurfaceCallbackT)(VkInstance, const VkAllocationCallbacks*, VkSurfaceKHR*);
typedef VkResult (*VKR_GetFramebufferSizeCallbackT)(unsigned*, unsigned*);
typedef const char* (*VKR_GetShaderBinaryCallbackT)(unsigned*);

typedef struct {
    glm::vec3 position;
    glm::vec3 color;
} VKRVertex;

typedef struct {
    VKRVertex vertices[3];
} VKRTriangle;
