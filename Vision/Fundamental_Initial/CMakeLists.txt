cmake_minimum_required(VERSION 3.5)
set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "") #Apple Silicon
project(Fundamental)

set(Vulkan_INCLUDE_DIR "/Users/xiyuxuan/VulkanSDK/include")
set(Vulkan_LIBRARY "/Users/xiyuxuan/VulkanSDK/lib")
set(CMAKE_PREFIX_PATH "/Users/xiyuxuan/Qt5.14.2")

# find_package(Qt5 COMPONENTS Core Widgets OpenGL REQUIRED)
set(QT_DIR "/Users/xiyuxuan/Qt5.14.2/5.14.2/clang_64/lib/cmake/Qt5")
set(Qt5_DIR "/Users/xiyuxuan/Qt5.14.2/5.14.2/clang_64/lib/cmake/Qt5")
if(Qt5_FOUND)
    message("${Qt5_VERSION}")
endif()

list(APPEND CMAKE_FRAMEWORK_PATH /Library/Frameworks) #Mac, why not auto?
find_package(Imagine REQUIRED COMPONENTS LinAlg Images)

add_executable(Fundamental
        Fundamental.cpp
        Imagine/SIFT_VL.cpp
        Imagine/vl/generic.c Imagine/vl/host.c Imagine/vl/imop.c Imagine/vl/sift.c)
target_link_libraries(Fundamental Imagine::LinAlg Imagine::Images)
