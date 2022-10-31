#ifndef MYENGINE_PLATFORM_H
#define MYENGINE_PLATFORM_H

// Introduction:
// This is the engine interface file. This is a C compatible header file
// which provides all the necessary functionality to interact with the
// rendering engine.
//
//
// Resources:
// Any engine allocated resource, i.e. a structure that returns a pointer
// is managed by the engine and will be automatically released when the
// application terminates.


// forward declarations for pointer types
struct EntityModel;
struct EntityTexture;
struct EntityInstance;
struct EngineCamera;

struct Engine {
    // Statistics
    bool  running;
    float uptime;
    float deltaTime;
    float elapsedFrames;
};


// Global scene information that will be accessed by the shaders to perform
// various computations. The order of the variables cannot be changed! This
// is because the shaders themselves directly replicate this structs order.
// Therefore, if this was to change then the shaders will set values for
// the wrong variables.
//
// Padding is equally important and hence the usage of the "alignas" keyword.
//
struct EngineScene {
    float ambientStrength;
    float specularStrength;
    float specularShininess;
    alignas(16) glm::vec3 cameraPosition;
    alignas(16) glm::vec3 lightPosition;
    alignas(16) glm::vec3 lightColor;

};

// This is the entry point for the engine and is where all initialization
// takes place. This should be the first function that gets called by
// the client application.
Engine* EngineStart(const char* name);

// This will terminate the engine by freeing all resources by the client and
// the engine then shutting down all subsystems. This should be the final engine
// function call within the client application.
void EngineStop();


// Returns a boolean value based on if the given key is currently
// pressed down.
bool EngineIsKeyDown(int keycode);

// Loads a model file from the filesystem and internally creates a render
// buffer which is returned to the client.
EntityModel* EngineLoadModel(const char* path);

// Loads a texture file from the filesystem and returns a pointer to that
// texture image.
EntityTexture* EngineLoadTexture(const char* path, VkFormat format);

// Creates an entity which is an object that is rendered onto the screen.
// Each entity has a pointer to a vertex buffer that describes the
// object that is being represented. A model matrix is also part of
// an entity that describes the full transformation including position,
// rotation and scale in the world.
EntityInstance* EngineCreateEntity(EntityModel* model,
                                   EntityTexture* texture,
                                   const glm::vec3& position = glm::vec3(0.0f),
                                   const glm::vec3& rotation = glm::vec3(0.0f),
                                   float scale = 1.0f);


EngineCamera* EngineCreateCamera(const glm::vec3& position, float fovy, float speed);

// Moves the default camera in the specified direction
void engine_move_forwards();
void engine_move_backwards();
void engine_move_left();
void engine_move_right();
void engine_move_up();
void engine_move_down();
void engine_roll_left();
void engine_roll_right();


void EngineTranslate(EntityInstance* e, const glm::vec3& position);
void EngineRotate(EntityInstance* e, float deg, const glm::vec3& axis);
void EngineScale(EntityInstance* e, float scale);
void EngineScale(EntityInstance* e, const glm::vec3& scale);
glm::vec3 EngineGetPosition(const EntityInstance* e);



void EngineBegin();
void EngineEnd();

void EngineRenderSkybox(EntityInstance* e);

// This function submits work to the GPU to execute. In other words, rendering
// an object onto the screen.
void EngineRender(EntityInstance* e);

void EngineRenderUI();

void EngineSubmit(const EngineScene& scene);


// For a given position in world space (X, Y, Z), the function will return the
// 2D screen coordinate. Often used for display a user interface that follows
// objects in the world.
glm::vec2 EngineWorldToScreen(const glm::vec3& position, const glm::vec2& offset = glm::vec2(0.0f));

// Camera
glm::vec3 EngineGetCameraPosition(const EngineCamera*);


std::string EngineLoadTextFile(std::string_view path);

#endif