#ifndef MYENGINE_PCH_HPP
#define MYENGINE_PCH_HPP

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <cstring>

#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <functional>
#include <regex>
#include <optional>
#include <set>
#include <array>
#include <map>
#include <filesystem>
#include <unordered_map>
#include <thread>
#include <future>
#include <format>
#include <tuple>


#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <xaudio2.h>
#include <x3daudio.h>

// NOTE: These must be placed after all Windows header files
#undef far
#undef near

#endif


// ensures that external code that calls vulkan.h does not give us symbol
// conflicts.
#define VK_NO_PROTOTYPES
#include <volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vk_mem_alloc.h>
#include <shaderc/shaderc.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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

//#define IMGUI_UNLIMITED_FRAME_RATE
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h> // Docking API
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <stb_image.h>


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>



#include <ImGuizmo.h>


#endif
