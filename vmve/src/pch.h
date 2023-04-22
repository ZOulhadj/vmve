#ifndef VMVE_PCH_H
#define VMVE_PCH_H

#include <string>
#include <fstream>
#include <ostream>
#include <vector>
#include <filesystem>
#include <array>
#include <expected>

#include <cassert>

//#define IMGUI_UNLIMITED_FRAME_RATE
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h> // Docking API
#include <imgui.h>
#include <imgui_stdlib.h>
#include <ImGuizmo.h>

// CryptoPP
#include <cryptopp/cryptlib.h>
#include <cryptopp/rijndael.h>
#include <cryptopp/modes.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <cryptopp/integer.h>
#include <cryptopp/dh.h>

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>

#include <engine.h>

#endif
