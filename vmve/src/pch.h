#ifndef VMVE_PCH_H
#define VMVE_PCH_H

#include <string>
#include <fstream>
#include <ostream>
#include <vector>
#include <filesystem>
#include <array>

#include <cassert>

//#define IMGUI_UNLIMITED_FRAME_RATE
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h> // Docking API
#include <imgui.h>
#include <ImGuizmo.h>





// CryptoPP
#include <cryptlib.h>
#include <rijndael.h>
#include <modes.h>
#include <files.h>
#include <osrng.h>
#include <hex.h>
#include <integer.h>
#include <dh.h>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>

#include <engine.h>

#endif
