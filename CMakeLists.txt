cmake_minimum_required(VERSION 3.23 FATAL_ERROR)
project(vmve
        VERSION 0.0.1
        DESCRIPTION "My custom rendering engine"
        LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

###############################
#       Compiler Options      #
###############################
set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_EXE_LINKER_FLAGS "-static")


include_directories(${PROJECT_NAME} PRIVATE src/)


###############################
#     3rd Party Libraries     #
###############################

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/vma)
add_subdirectory(vendor/assimp)
add_subdirectory(vendor/cryptopp)

include_directories(IMGUI_INCLUDE vendor/imgui/)
set(IMGUI_SRC
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_demo.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_impl_glfw.cpp
        vendor/imgui/imgui_impl_vulkan.cpp
        vendor/imgui/imgui_tables.cpp
        vendor/imgui/imgui_widgets.cpp)

set(IMGUIZMO_SRC
        vendor/imgui/ImGuizmo.cpp)

include_directories(STB_INCLUDE vendor/stb/)



add_executable(${PROJECT_NAME}
        ${IMGUI_SRC}
        ${IMGUIZMO_SRC}

        # engine test file
        "sandbox/main.cpp"
        "src/core/window.cpp"
        "src/rendering/camera.cpp"
        "src/rendering/entity.cpp"
        "src/core/input.cpp"
        "src/rendering/model.cpp"
        "src/rendering/vertex.cpp"
        "src/filesystem/vfs.cpp"
        "src/filesystem/filesystem.cpp"
        "src/utility.cpp"
        "src/rendering/material.cpp"
        "src/rendering/vulkan/renderer.cpp"
        "src/ui/ui.cpp"
        "src/logging.cpp"
        "src/rendering/vulkan/vertex_array.cpp"
        "src/rendering/vulkan/buffer.cpp"
        "src/rendering/vulkan/shader.cpp"
        "src/rendering/vulkan/context.cpp"
        "src/rendering/vulkan/texture.cpp"
        "src/rendering/vulkan/descriptor_sets.cpp"
        "src/core/platform/windows.cpp"
        "src/time.cpp"
        "src/core/command_line.cpp"

        # precompiled header
        # note: this is required to compile header only files such as STB, VMA
        # note: this will be removed in the future.
        "src/pch.cpp" "src/rendering/directx/context.cpp")
        
     


target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${STB_INCLUDE} ${IMGUI_INCLUDE})
target_link_directories(${PROJECT_NAME} PRIVATE "C:/VulkanSDK/1.3.231.1/Lib")
target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        glm::glm
        Vulkan::Vulkan
        shaderc_shared
        VulkanMemoryAllocator
        assimp
        cryptopp
        )
