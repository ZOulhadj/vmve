cmake_minimum_required(VERSION 3.23 FATAL_ERROR)
project(my_engine
        VERSION 0.0.1
        DESCRIPTION "My custom rendering engine"
        LANGUAGES CXX)

###############################
#       Compiler Options      #
###############################
set(CMAKE_CXX_STANDARD 23)
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
add_subdirectory(vendor/vma)
add_subdirectory(vendor/tiny_obj_loader)
add_subdirectory(vendor/assimp)
add_subdirectory(vendor/json)

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
        "sandbox/Main.cpp"

        # engine source
        "src/window.cpp"
        "src/Camera.cpp"
        "src/Entity.cpp"
        "src/Input.cpp"

        # engine rendering
        "src/Renderer/Renderer.cpp"
        "src/Renderer/UI.cpp"

        # precompiled header
        # note: this is required to compile header only files such as STB
        # note: this will be removed in the future.
        "src/PCH.cpp"
        "src/Model.cpp"
        "src/Renderer/Texture.cpp"
        "src/Vertex.cpp"
        "src/Renderer/VertexArray.cpp"
        "src/Renderer/Buffer.cpp"
        "src/Renderer/Shader.cpp"
        "src/Renderer/RendererContext.cpp")


target_precompile_headers(${PROJECT_NAME} PRIVATE
        src/PCH.hpp)

target_include_directories(${PROJECT_NAME} PRIVATE
        vendor/stb
        vendor/imgui)

target_link_directories(${PROJECT_NAME} PRIVATE
        ${Vulkan_INCLUDE_DIR}/../Lib)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        glfw
        glm::glm
        Vulkan::Vulkan
        shaderc_shared
        VulkanMemoryAllocator
        tinyobjloader
        assimp
        nlohmann_json::nlohmann_json)
