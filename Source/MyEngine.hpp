#pragma once

struct VertexBuffer;
struct TextureBuffer;
struct Entity;


enum CameraDirections
{
    camera_forward,
    camera_backwards,
    camera_left,
    camera_right,
    camera_up,
    camera_down,
    camera_roll_left,
    camera_roll_right
};

namespace Engine
{
    // This is the entry point for the engine and is where all initialization
    // takes place. This should be the first function that gets called by
    // the client application.
    void Start(const char* name);

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
    float DeltaTime();

    // Returns a boolean value based on if the given key is currently
    // pressed down.
    bool IsKeyDown(int keycode);

    // Returns a boolean value based on if the given mouse button
    // is pressed down.
    bool IsMouseButtonDown(int buttoncode);

//    vec2 mouse_position();

    // Creates a region of memory stored on the GPU. The object returns
    // a pointer to a vertex and index buffer which can be used for rendering.
    VertexBuffer* CreateVertexBuffer(void* v, int vs, void* i, int is);

    // Loads a model file from the filesystem and internally creates a render
    // buffer which is returned to the client.
    VertexBuffer* LoadModel(const char* path);

    // Loads a texture file from the filesystem and returns a pointer to that
    // texture image.
    TextureBuffer* LoadTexture(const char* path);

    // Creates an entity which is an object that is rendered onto the screen.
    // Each entity has a pointer to a vertex buffer that describes the
    // object that is being represented. A model matrix is also part of
    // an entity that describes the full transformation including position,
    // rotation and scale in the world.
    Entity* CreateEntity(const VertexBuffer* vertexBuffer);

    // Moves the default camera in the specified direction
    void MoveCamera(CameraDirections direction);

    // This function performs all the required work per frame and must be called
    // first before any other rendering related function call.
    void BeginRender();

    // todo: Most likely will be moved internally as the client should not need to
    // todo: be aware of the different render passes.
    void BeginRenderPass();
    void EndRenderPass();

    void BindBuffer(const VertexBuffer* buffer);

    void BindPipeline();

    // This function submits work to the GPU to execute. In other words, rendering
    // an object onto the screen.
    void Render(Entity* e);

    //
    void RenderDebugUI();

    // Once all rendering commands have been completed, this function can be called
    // which will submit all commands to the GPU to be rendered and displayed onto
    // the screen.
    void EndRender();

    void TranslateEntity(Entity* e, float x, float y, float z);
    void RotateEntity(Entity* e, float deg, float x, float y, float z);
    void ScaleEntity(Entity* e, float scale);
    void ScaleEntity(Entity* e, float x, float y, float z);

    void GetEntityPosition(const Entity* e, float* x, float* y, float* z);
}




