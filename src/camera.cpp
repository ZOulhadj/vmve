#include "camera.hpp"


// Reversed Z infinite perspective
// GLM provides various types of perspective functions including infinitePerspective()
// but a reversed depth version (far-near) is not. Thus, below is my own version
// that creates an infinite perspective from far to near.
glm::mat4 infinite_perspective(float fovy, float width, float height, float zNear)
{
	const float h = glm::cot(0.5f * fovy);
	const float w = h * height / width;

    glm::mat4 result = glm::mat4(0.0f);
	result[0][0] = w;
	result[1][1] = h;
	result[2][2] = 0.0f;
	result[2][3] = 1.0f;
	result[3][2] = zNear;

    return result;
}

camera_t create_camera(const glm::vec3& position, float fov, float speed)
{
    camera_t camera{};
    camera.position    = position;
    camera.orientation = glm::quat(1, 0, 0, 0);
    camera.width       = 1280.0f;
    camera.height      = 720.0f;
    camera.speed       = speed;
    camera.view_speed  = 0.1f;
    camera.roll_speed  = 45.0f;
    camera.roll        = 0.0f;
    camera.fov         = fov;
    camera.near        = 0.1f;
    camera.viewProj.view = glm::mat4_cast(glm::quat(camera.orientation)) * glm::translate(glm::mat4(1.0f), -glm::vec3(camera.position));
    camera.viewProj.proj = infinite_perspective(glm::radians(camera.fov),
                                                camera.width, camera.height,
                                                camera.near);

    // Required if using Vulkan (left-handed coordinate-system)
    camera.viewProj.proj[1][1] *= -1.0;

    return camera;
}

void update_camera_view(camera_t& camera, float cursor_x, float cursor_y)
{
    camera.cursor_x = cursor_x;
    camera.cursor_y = cursor_y;
}

void update_camera(camera_t& camera)
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
    camera.viewProj.view = glm::mat4_cast(glm::quat(camera.orientation)) * glm::translate(glm::mat4(1.0f), -glm::vec3(camera.position));

    // Build final camera transform matrix
    camera.roll      = 0.0f;
}

void update_projection(camera_t& camera)
{
    camera.viewProj.proj = infinite_perspective(glm::radians(camera.fov),
                                                camera.width, camera.height,
                                                camera.near);
    // Required if using Vulkan (left-handed coordinate-system)
    camera.viewProj.proj[1][1] *= -1.0;
}

void set_camera_projection(camera_t& camera, uint32_t width, uint32_t height)
{
    update_projection(camera);
}
