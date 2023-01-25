#ifndef MY_ENGINE_QUATERNION_CAMERA_HPP
#define MY_ENGINE_QUATERNION_CAMERA_HPP


struct Frustum_Plane {
    glm::vec3 normal;
    float distance_from_origin;
};

struct Frustum {
    Frustum_Plane top;
    Frustum_Plane bottom;
    Frustum_Plane left;
    Frustum_Plane right;
    Frustum_Plane near;
    Frustum_Plane far;
};


enum class Camera_Projection {
    perspective,
    orthographic
};

enum class Camera_Type {
    first_person,
    look_at
};

struct View_Projection {
    glm::mat4 view;
    glm::mat4 proj;
};

struct Camera {
    Camera_Type type;
    Camera_Projection projection;


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

    float near;
    float far;

    View_Projection viewProj;

    bool first_mouse;
};


Frustum create_camera_frustum(const Camera& camera);

Camera create_perspective_camera(Camera_Type type, const glm::vec3& position, float fov, float speed);
Camera create_orthographic_camera(Camera_Type type, const glm::vec3& position, float speed);

//Camera CreateCamera(const glm::vec3& position, float fov, float speed);
void update_camera(Camera& camera, const glm::vec2& cursor_pos);
void update_projection(Camera& cam);
void update_projection(Camera& camera, uint32_t width, uint32_t height);


#endif
