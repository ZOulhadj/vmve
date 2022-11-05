#ifndef MYENGINE_QUATERNION_CAMERA_HPP
#define MYENGINE_QUATERNION_CAMERA_HPP


struct view_projection {
    glm::mat4 view;
    glm::mat4 proj;
};

struct QuatCamera {
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


    float cursor_x;
    float cursor_y;
};


QuatCamera CreateCamera(const glm::vec3& position, float fov, float speed);

void UpdateCameraView(QuatCamera& camera, float cursor_x, float cursor_y);
void UpdateCamera(QuatCamera& camera);

void update_projection(QuatCamera& camera);
void UpdateCameraProjection(QuatCamera& camera, uint32_t width, uint32_t height);


#endif
