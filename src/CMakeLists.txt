cmake_minimum_required(VERSION 3.12)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

set(VKR_INTERFACE_HEADERS
    ../include/VKR/VKR.hpp
    ../include/VKR/VKRVulkan.hpp
)

set(VKR_SOURCES 
    Buffer.cpp
    GraphicsDevice.cpp
    Image.cpp
    Instance.cpp
    Material.cpp
    Mesh.cpp
    Model.cpp
    Scene.cpp
    Sync.cpp
    WSISwapchain.cpp
)

add_library(VKR
    ${VKR_SOURCES}
    ${VKR_INTERFACE_HEADERS}
)
target_include_directories(VKR
    PRIVATE ../include/VKR
    INTERFACE ../include
)
target_link_libraries(VKR PUBLIC glm::glm PRIVATE Vulkan::Vulkan VMA)
target_compile_features(VKR PUBLIC cxx_std_20)

add_library(VKRVulkan INTERFACE)
target_link_libraries(VKRVulkan INTERFACE VKR Vulkan::Vulkan)
