#ifndef MYENGINE_QUATERNION_CAMERA_HPP
#define MYENGINE_QUATERNION_CAMERA_HPP


struct view_projection
{
    glm::mat4 view;
    glm::mat4 proj;
};

struct camera_t
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

    view_projection viewProj;


    float cursor_x;
    float cursor_y;
};


camera_t create_camera(const glm::vec3& position, float fov, float speed);

void update_camera_view(camera_t& camera, float cursor_x, float cursor_y);
void update_camera(camera_t& camera);

void update_projection(camera_t& camera);
void set_camera_projection(camera_t& camera, uint32_t width, uint32_t height);


#endif
