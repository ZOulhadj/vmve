#ifndef MY_ENGINE_CAMERA_H
#define MY_ENGINE_CAMERA_H

#include "entity.h"

namespace engine {
    enum class Camera_Type
    {
        first_person,
        look_at
    };

    struct View_Projection
    {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Camera
    {
        Camera_Type type;

        glm::vec3 position;
        glm::vec3 front_vector;
        glm::vec3 right_vector;
        glm::vec3 up_vector;
        float roll;
        glm::quat orientation;

        float speed;
        float view_speed;
        float roll_speed;
        float fov;

        float near_plane;
        float far_plane;

        View_Projection vp;

        bool first_mouse;
    };

    Camera create_perspective_camera(const glm::vec3& position, float fov, float speed);

    void update_camera_view(Camera& camera, const glm::vec2& cursor_pos);
    void update_projection(Camera& camera, uint32_t width, uint32_t height);
}



#endif
