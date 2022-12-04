#ifndef MY_ENGINE_MODEL_HPP
#define MY_ENGINE_MODEL_HPP


#include "renderer/vertex_array.hpp"
#include "material.hpp"


struct model_t {
    vertex_array_t data;
    material_t textures;
};

vertex_array_t load_model(const std::string& path);
model_t load_model_new(const std::string& path);

#endif
