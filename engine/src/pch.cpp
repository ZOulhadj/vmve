#include "pch.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>


#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <tiny_gltf.h>

//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
