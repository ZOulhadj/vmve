#ifndef MY_ENGINE_CAMERA_H
#define MY_ENGINE_CAMERA_H

#include "entity.h"

#include "events/window_event.h"

// Windows craziness
#undef near
#undef far

namespace engine {

#if 0
    struct camera_transform
    {
        glm::mat4 view;
        glm::mat4 projection;

        camera_transform()
            : view(glm::mat4(1.0f)),
            projection(glm::mat4(1.0f))
        {}
    };

    struct camera_clip
    {
        float near;
        float far;
    };

    class perspective_camera
    {
    public:
        perspective_camera(const glm::vec3& position, float speed, float fov, camera_clip clip);

        void on_update(float delta_time);
        void on_resize(window_resized_event& e);

        camera_transform get_transform();
        glm::vec3 get_position();
    private:
        camera_transform m_transform;

        glm::vec3 m_position;
        glm::vec3 m_up_vector;
        glm::vec3 m_front_vector;

        camera_clip m_clip;
        float m_speed;
        float m_fov;
    };
#endif
#if 1

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

#endif
}



#endif
