#ifndef MY_ENGINE_PCH_H
#define MY_ENGINE_PCH_H

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <string_view>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <functional>
#include <optional>
#include <array>
#include <filesystem>
#include <set>
#include <expected>

// ensures that external code that calls vulkan.h does not give us symbol
// conflicts.

#define VK_NO_PROTOTYPES

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#define GLFW_INCLUDE_NONE
#if defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

//#define IMGUI_UNLIMITED_FRAME_RATE
//#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES
#define IMGUI_DEFINE_MATH_OPERATORS



#include <volk.h>
#include <vk_mem_alloc.h>
#include <shaderc/shaderc.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/reciprocal.hpp> // glm::cot
#include <glm/gtc/type_ptr.hpp>
#include <glm/exponential.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/euler_angles.hpp>


#include <imgui_internal.h> // Docking API
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <ImGuizmo.h>

#include <tiny_gltf.h>

#include <entt.hpp>

#endif
