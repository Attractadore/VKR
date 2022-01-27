add_library(VMA vma.cpp)
target_include_directories(VMA PUBLIC "VulkanMemoryAllocator/include")
target_compile_features(VMA PRIVATE cxx_std_14)
