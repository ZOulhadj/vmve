#ifndef MYENGINE_QUATERNION_CAMERA_HPP
#define MYENGINE_QUATERNION_CAMERA_HPP

struct quaternion_camera
{
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
    //float far;

    glm::mat4 view;
    glm::mat4 proj;


    float cursor_x;
    float cursor_y;
};


quaternion_camera create_camera(const glm::vec3& position, float fov, float speed);

void update_camera_view(quaternion_camera& camera, float cursor_x, float cursor_y);
void update_camera(quaternion_camera& camera);

void update_projection(quaternion_camera& camera);
void update_camera_projection(quaternion_camera& camera, uint32_t width, uint32_t height);


#endif
