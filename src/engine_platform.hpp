#pragma once

struct vertex_buffer;
struct texture_buffer;
struct entity;

namespace engine
{
    // This is the entry point for the engine and is where all initialization
    // takes place. This should be the first function that gets called by
    // the client application.
    void start(const char* name);

    // This will terminate the engine by freeing all resources by the client and
    // the engine then shutting down all subsystems. This should be the final engine
    // function call within the client application.
    void Exit();

    // This function simply sets the running status of the engine to false which
    // in turn will exit the main rendering loop once running() is called.
    void Stop();

    // Returns the running status of the engine. Once the engine has been
    // initialized then this is set to true. The client can use this for the
    // main update loop.
    bool Running();

    // The uptime returns the number of seconds since the engine
    // was initialized.
    float Uptime();

    // The delta time will return the time difference in milliseconds
    // between the current and previous frame.
    float get_delta_time();

    // Returns a boolean value based on if the given key is currently
    // pressed down.
    bool is_key_down(int keycode);

    // Returns a boolean value based on if the given mouse button
    // is pressed down.
    bool is_mouse_button_down(int buttoncode);

    // Creates a region of memory stored on the GPU. The object returns
    // a pointer to a vertex and index buffer which can be used for rendering.
    vertex_buffer* create_render_buffer(void* v, int vs, void* i, int is);

    // Loads a model file from the filesystem and internally creates a render
    // buffer which is returned to the client.
    vertex_buffer* load_model(const char* path);

    // Loads a texture file from the filesystem and returns a pointer to that
    // texture image.
    texture_buffer* load_texture(const char* path);

    // Creates an entity which is an object that is rendered onto the screen.
    // Each entity has a pointer to a vertex buffer that describes the
    // object that is being represented. A model matrix is also part of
    // an entity that describes the full transformation including position,
    // rotation and scale in the world.
    entity* create_entity(const vertex_buffer* vertexBuffer);

    // Moves the default camera in the specified direction
    void MoveForward();
    void MoveBackwards();
    void MoveLeft();
    void MoveRight();
    void MoveUp();
    void MoveDown();
    void RollLeft();
    void RollRight();

    void render();

    // This function submits work to the GPU to execute. In other words, rendering
    // an object onto the screen.
    void Render(entity* e);

    void TranslateEntity(entity* e, float x, float y, float z);
    void RotateEntity(entity* e, float deg, float x, float y, float z);
    void ScaleEntity(entity* e, float scale);
    void ScaleEntity(entity* e, float x, float y, float z);

    void GetEntityPosition(const entity* e, float* x, float* y, float* z);
}




