#ifndef MY_ENGINE_QUATERNION_CAMERA_HPP
#define MY_ENGINE_QUATERNION_CAMERA_HPP


struct frustum_plane {
    glm::vec3 normal;
    float distance_from_origin;
};

struct camera_frustum {
    frustum_plane top;
    frustum_plane bottom;
    frustum_plane left;
    frustum_plane right;
    frustum_plane near;
    frustum_plane far;
};


enum class camera_projection_mode {
    perspective,
    ortho
};

enum class camera_mode {
    first_person,
    look_at
};

struct view_projection {
    glm::mat4 view;
    glm::mat4 proj;
};

struct camera_t {
    glm::vec3 position;
    glm::vec3 front_vector;
    glm::vec3 right_vector;
    glm::vec3 up_vector;
    float roll;
    glm::quat orientation;


    //float aspect_ratio;
    float width;
    float height;


    float speed;
    float view_speed;
    float roll_speed;
    float fov;

    float near;

    view_projection viewProj;

    bool first_mouse;
};


camera_frustum create_camera_frustum(const camera_t& camera);

camera_t create_camera(const glm::vec3& position, float fov, float speed);
void update_camera(camera_t& camera, const glm::vec2& cursor_pos);
void update_projection(camera_t& cam);
void set_camera_projection(camera_t& camera, uint32_t width, uint32_t height);


#endif
