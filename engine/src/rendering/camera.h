#ifndef MY_ENGINE_CAMERA_H
#define MY_ENGINE_CAMERA_H

#include "entity.h"

struct Frustum_Plane
{
    glm::vec3 normal;
    float distance_from_origin;
};

struct Frustum
{
    Frustum_Plane top;
    Frustum_Plane bottom;
    Frustum_Plane left;
    Frustum_Plane right;
    Frustum_Plane near_plane;
    Frustum_Plane far_plane;
};

enum class camera_type
{
    first_person,
    look_at
};

struct view_projection
{
    glm::mat4 view;
    glm::mat4 proj;
};

struct Camera
{
    camera_type type;

    glm::vec3 position;
    glm::vec3 front_vector;
    glm::vec3 right_vector;
    glm::vec3 up_vector;
    float roll;
    glm::quat orientation;


    //float aspect_ratio;
    uint32_t width;
    uint32_t height;


    float speed;
    float view_speed;
    float roll_speed;
    float fov;

    float near_plane;
    float far_plane;

    view_projection view_proj;

    bool first_mouse;
};


Frustum create_camera_frustum(const Camera& camera);

Camera create_perspective_camera(const glm::vec3& position, float fov, float speed);

void update_camera(Camera& camera, const glm::vec2& cursor_pos);
void update_projection(Camera& cam);
void update_projection(Camera& camera, uint32_t width, uint32_t height);


#endif
