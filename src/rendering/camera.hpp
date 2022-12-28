#ifndef MY_ENGINE_QUATERNION_CAMERA_HPP
#define MY_ENGINE_QUATERNION_CAMERA_HPP


struct FrustumPlane {
    glm::vec3 normal;
    float distance_from_origin;
};

struct Frustum {
    FrustumPlane top;
    FrustumPlane bottom;
    FrustumPlane left;
    FrustumPlane right;
    FrustumPlane near;
    FrustumPlane far;
};


enum class ProjectionMode {
    perspective,
    ortho
};

enum class CameraType {
    FirstPerson,
    LookAt
};

struct ViewProjection {
    glm::mat4 view;
    glm::mat4 proj;
};

struct Camera {
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

    ViewProjection viewProj;

    bool first_mouse;
};


Frustum CreateCameraFrustum(const Camera& camera);

Camera CreateCamera(const glm::vec3& position, float fov, float speed);
void UpdateCamera(Camera& camera, const glm::vec2& cursor_pos);
void UpdateProjection(Camera& cam);
void UpdateProjection(Camera& camera, uint32_t width, uint32_t height);


#endif
