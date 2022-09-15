cmake_minimum_required(VERSION 3.23)
project(MyEngine)

###############################
#       Compiler Options      #
###############################
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXE_LINKER_FLAGS "-static")

###############################
#     3rd Party Libraries     #
###############################

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(Vendor/glfw)
add_subdirectory(Vendor/glm)

set(IMGUI_SRC
        Vendor/imgui/imgui.cpp
        Vendor/imgui/imgui_demo.cpp
        Vendor/imgui/imgui_draw.cpp
        Vendor/imgui/imgui_impl_glfw.cpp
        Vendor/imgui/imgui_impl_vulkan.cpp
        Vendor/imgui/imgui_tables.cpp
        Vendor/imgui/imgui_widgets.cpp)

add_executable(${PROJECT_NAME}
        ${IMGUI_SRC}

        # Engine test file
        MyEngineTest/Main.cpp

        # Engine source
        Source/Window.cpp
        Source/Renderer.cpp
        Source/Camera.cpp
        Source/Entity.cpp
        Source/MyEngine.cpp)


target_include_directories(${PROJECT_NAME} PRIVATE
        Vendor/stb
        Vendor/tol
        Vendor/imgui
        Vendor/vma)
target_link_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIR}/../Lib)
target_link_libraries(${PROJECT_NAME}
        glfw
        glm::glm
        Vulkan::Vulkan
        shaderc_shared)
