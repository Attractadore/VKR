#pragma once
#include "VKR.hpp"

#include <vulkan/vulkan.h>

#include <array>

namespace VKR {
constexpr std::array color_fmts = {
    VK_FORMAT_R16G16B16_UNORM,
    VK_FORMAT_R16G16B16_SFLOAT,
    VK_FORMAT_R16G16B16A16_UNORM,
    VK_FORMAT_R16G16B16A16_SFLOAT,
};

constexpr std::array depth_fmts = {
    VK_FORMAT_X8_D24_UNORM_PACK32,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D16_UNORM,
};

constexpr std::array srgb_fmts = {
    VK_FORMAT_B8G8R8_SRGB,
    VK_FORMAT_R8G8B8_SRGB,
    VK_FORMAT_A8B8G8R8_SRGB_PACK32,
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_R8G8B8A8_SRGB,
};
}
