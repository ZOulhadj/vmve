#include "quaternion_camera.hpp"

quaternion_camera create_camera(const glm::vec3& position, float fov, float speed)
{
    quaternion_camera camera{};
    camera.position    = position;
    camera.orientation = glm::quat(1, 0, 0, 0);
    camera.aspect_ratio = 1280.0f / 720.0f;
    camera.speed       = speed;
    camera.view_speed  = 0.1f;
    camera.roll_speed  = 180.0f;
    camera.roll        = 0.0f;
    camera.fov         = fov;
    camera.near        = 0.1f;
    camera.far         = 10000.0f;

    camera.view = glm::mat4_cast(glm::quat(camera.orientation)) * glm::translate(glm::mat4(1.0f), -glm::vec3(camera.position));
    camera.proj = glm::perspective(glm::radians(camera.fov), camera.aspect_ratio, camera.far, camera.near);

    // Required if using Vulkan (left-handed coordinate-system)
    camera.proj[1][1] *= -1.0;

    return camera;
}

void update_camera_view(quaternion_camera& camera, float cursor_x, float cursor_y)
{
    camera.cursor_x = cursor_x;
    camera.cursor_y = cursor_y;
}

void update_camera(quaternion_camera& camera)
{
    // todo(zak): Need to fix unwanted roll when rotating
    // Get the mouse offsets
    static float last_x = 0.0f;
    static float last_y = 0.0f;
    const float xoffset = (last_x - camera.cursor_x) * camera.view_speed;
    const float yoffset = (last_y - camera.cursor_y) * camera.view_speed;
    last_x = camera.cursor_x;
    last_y = camera.cursor_y;

    // Get the camera current direction vectors based on orientation
    camera.front_vector = glm::conjugate(camera.orientation) * glm::vec3(0.0f, 0.0f, 1.0f);
    camera.up_vector    = glm::conjugate(camera.orientation) * glm::vec3(0.0f, 1.0f, 0.0f);
    camera.right_vector = glm::conjugate(camera.orientation) * glm::vec3(1.0f, 0.0f, 0.0f);

    // Create each of the orientations based on mouse offset and roll offset.
    //
    // This code snippet below locks the yaw to world coordinates.
    // glm::quat yaw   = glm::angleAxis(glm::radians(xoffset), cam.orientation * glm::vec3(0.0f, 1.0f, 0.0f))
    const glm::quat pitch = glm::angleAxis(glm::radians(yoffset),  glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat yaw   = glm::angleAxis(glm::radians(xoffset),  glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat roll  = glm::angleAxis(glm::radians(camera.roll), glm::vec3(0.0f, 0.0f, 1.0f));

    // Update the camera orientation based on pitch, yaw and roll
    camera.orientation = (pitch * yaw * roll) * camera.orientation;

    // Create the view and projection matrices
    camera.view = glm::mat4_cast(glm::quat(camera.orientation)) * glm::translate(glm::mat4(1.0f), -glm::vec3(camera.position));

    // Build final camera transform matrix
    //cam.view = proj * view;
    camera.roll      = 0.0f;
}

static void update_projection(quaternion_camera& camera)
{
    camera.proj = glm::perspective(glm::radians(camera.fov), camera.aspect_ratio, camera.far, camera.near);
    // Required if using Vulkan (left-handed coordinate-system)
    camera.proj[1][1] *= -1.0;
}

void update_camera_projection(quaternion_camera& camera, uint32_t width, uint32_t height)
{
    //cam.proj = glm::infinitePerspective(glm::radians<double>(cam.fov), (double)gWindow->width / gWindow->height, 0.1);
    camera.aspect_ratio = (float)width / (float)height;

    update_projection(camera);
}