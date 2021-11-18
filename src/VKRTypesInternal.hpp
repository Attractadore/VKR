#pragma once
#include "VKRMesh.hpp"

#include <glm/mat4x4.hpp>

typedef struct {
    VkBuffer buf;
    VkDeviceMemory mem;
    unsigned size;
} VKRBuffer;

typedef struct {
    VkImage img;
    VkDeviceMemory mem;
} VKRImage;
