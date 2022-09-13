#ifndef MYENGINE_CAMERA_HPP
#define MYENGINE_CAMERA_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct QuaternionCamera
{
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

    glm::dmat4 view;
    glm::dmat4 proj;
};


QuaternionCamera CreateCamera(const glm::vec3& position, float fov, float speed);
void UpdateCamera(QuaternionCamera& camera);



#endif
