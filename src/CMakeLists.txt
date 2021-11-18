find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
if (NOT TARGET glm::glm)
    add_library(glm::glm ALIAS glm)
endif()

add_library(VKR
    VKR.cpp
    VKRCommands.cpp
    VKRDebugMessenger.cpp
    VKRDevice.cpp
    VKRInstance.cpp
    VKRMemory.cpp
    VKRMesh.cpp
    VKRPipeline.cpp
    VKRRenderPass.cpp
    VKRState.cpp
    VKRSurface.cpp
    VKRSwapchain.cpp
    VKRSynchronization.cpp
    VKRUtils.cpp
)
target_include_directories(VKR
    INTERFACE ../include/
    PRIVATE ../include/VKR
)
target_link_libraries(VKR PUBLIC Vulkan::Vulkan glm::glm)
