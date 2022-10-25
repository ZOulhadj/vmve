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
struct VertexBuffer;
struct TextureBuffer;
struct Entity;

struct Engine
{




    // Statistics
    bool  Running;
    float Uptime;
    float DeltaTime;
    float ElapsedFrames;
};


struct EngineScene
{
    alignas(16) glm::vec3 camera_position;
    alignas(16) glm::vec3 sun_position;
    alignas(16) glm::vec3 sun_color;
};

// This is the entry point for the engine and is where all initialization
// takes place. This should be the first function that gets called by
// the client application.
Engine* StartEngine(const char* name);

// This will terminate the engine by freeing all resources by the client and
// the engine then shutting down all subsystems. This should be the final engine
// function call within the client application.
void StopEngine();


// Returns a boolean value based on if the given key is currently
// pressed down.
bool EngineIsKeyDown(int keycode);

// Loads a model file from the filesystem and internally creates a render
// buffer which is returned to the client.
VertexBuffer* EngineLoadModel(const char* path);

// Loads a texture file from the filesystem and returns a pointer to that
// texture image.
TextureBuffer* EngineLoadTexture(const char* path);

// Creates an entity which is an object that is rendered onto the screen.
// Each entity has a pointer to a vertex buffer that describes the
// object that is being represented. A model matrix is also part of
// an entity that describes the full transformation including position,
// rotation and scale in the world.
Entity* EngineCreateEntity(const VertexBuffer* buffer, const TextureBuffer* texture);

// Moves the default camera in the specified direction
void engine_move_forwards();
void engine_move_backwards();
void engine_move_left();
void engine_move_right();
void engine_move_up();
void engine_move_down();
void engine_roll_left();
void engine_roll_right();

void EngineRender(const EngineScene& scene);

// This function submits work to the GPU to execute. In other words, rendering
// an object onto the screen.
void EngineRender(Entity* e);

void EngineTranslateEntity(Entity* e, float x, float y, float z);
void EngineRotateEntity(Entity* e, float deg, float x, float y, float z);
void EngineScaleEntity(Entity* e, float scale);
void EngineScaleEntity(Entity* e, float x, float y, float z);

void engine_get_entity_position(const Entity* e, float* x, float* y, float* z);



// Window
glm::vec2 get_window_size();

// Camera
glm::mat4 get_camera_projection();
glm::mat4 get_camera_view();
glm::vec3 get_camera_position();

#endif