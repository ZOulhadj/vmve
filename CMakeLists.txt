cmake_minimum_required(VERSION 3.23 FATAL_ERROR)
project(my_engine
        VERSION 0.0.1
        DESCRIPTION "My custom rendering engine"
        LANGUAGES CXX)

###############################
#       Compiler Options      #
###############################
set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_EXE_LINKER_FLAGS "-static")

###############################
#     3rd Party Libraries     #
###############################

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glm)

set(IMGUI_SRC
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_demo.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_impl_glfw.cpp
        vendor/imgui/imgui_impl_vulkan.cpp
        vendor/imgui/imgui_tables.cpp
        vendor/imgui/imgui_widgets.cpp)

add_executable(${PROJECT_NAME}
        ${IMGUI_SRC}

        # engine test file
        my_engine_test/main.cpp

        # engine source
        src/window.cpp
        src/vulkan_renderer.cpp
        src/quaternion_camera.cpp
        src/entity.cpp
        src/engine_platform.cpp)


#target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)
target_include_directories(${PROJECT_NAME} PRIVATE
        vendor/stb
        vendor/tol
        vendor/imgui
        vendor/vma)
target_link_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIR}/../Lib)
target_link_libraries(${PROJECT_NAME}
        glfw
        glm::glm
        Vulkan::Vulkan
        shaderc_shared)
