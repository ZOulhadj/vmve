#ifndef MY_ENGINE_CAMERA_H
#define MY_ENGINE_CAMERA_H

#include "entity.h"

#include "events/window_event.h"

// Windows craziness
#undef near
#undef far

namespace engine {

    struct camera_projection {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct camera_frustum {
        glm::vec4 left;
        glm::vec4 right;
        glm::vec4 top;
        glm::vec4 bottom;
        glm::vec4 near;
        glm::vec4 far;
    };

    struct perspective_camera {
        glm::vec3 position;
        glm::vec3 front_vector;
        glm::vec3 up_vector;
        glm::quat orientation;

        camera_projection vp;
        camera_frustum frustum;

        float fovy;
        float near_plane;
        float far_plane;
        float speed;
        float view_speed;
        float roll;
        float roll_speed;

        bool first_mouse;
    };

    perspective_camera create_camera(const glm::vec3& position, float fov, float speed);

    void update_camera_view(perspective_camera& camera, const glm::vec2& cursor_pos);
    void update_projection(perspective_camera& camera, uint32_t width, uint32_t height);

    camera_frustum extract_frustum_planes(const glm::mat4& matrix);

}



#endif
