#include "pch.h"
#include "camera.h"


#include "core/platform_input.h"

namespace engine {
    perspective_camera create_camera(const glm::vec3& position, float fov, float speed)
    {
        perspective_camera cam{};
        cam.position = position;
        cam.orientation = glm::quat(1, 0, 0, 0);
        cam.speed = speed;
        cam.view_speed = 0.1f;
        cam.roll_speed = 45.0f;
        cam.roll = 0.0f;
        cam.fovy = fov;
        cam.near_plane = 0.1f;
        cam.far_plane = 2000.0f;


        cam.vp.view = glm::mat4_cast(glm::quat(cam.orientation)) * glm::translate(glm::mat4(1.0f), -glm::vec3(cam.position));
        cam.vp.proj = glm::perspective(glm::radians(fov), 1280.0f / 720.0f, cam.far_plane, cam.near_plane);

        // Required if using Vulkan (left-handed coordinate-system)
        cam.vp.proj[1][1] *= -1.0;

        return cam;
    }

    // Reversed Z infinite perspective
    // GLM provides various types of perspective functions including infinitePerspective()
    // but a reversed depth version (far-near) is not. Thus, below is my own version
    // that creates an infinite perspective from towards near without a far plane.
    //glm::mat4 infinite_perspective(float fovy, const glm::vec2& size, float near)
    //{
    //	const float h = glm::cot(0.5f * fovy);
    //	const float w = h * size.y / size.x;
    //
    //    glm::mat4 result = glm::mat4(0.0f);
    //	result[0][0] = w;
    //	result[1][1] = h;
    //	result[2][2] = 0.0f;
    //	result[2][3] = 1.0f;
    //	result[3][2] = near;
    //
    //    return result;
    //}
    //

    void update_camera_view(perspective_camera& camera, const glm::vec2& cursor_pos)
    {
        // todo(zak): Need to fix unwanted roll when rotating
        static float last_x = 1280.0f / 2.0f;
        static float last_y = 720.0f / 2.0f;

        if (camera.first_mouse) {
            last_x = cursor_pos.x;
            last_y = cursor_pos.y;
            camera.first_mouse = false;
        }

        const float xoffset = (last_x - cursor_pos.x) * camera.view_speed;
        const float yoffset = (last_y - cursor_pos.y) * camera.view_speed;

        last_x = cursor_pos.x;
        last_y = cursor_pos.y;

#if 1
        // Get the camera current direction vectors based on orientation
        camera.front_vector = glm::conjugate(camera.orientation) * glm::vec3(0.0f, 0.0f, 1.0f);
        camera.up_vector = glm::conjugate(camera.orientation) * glm::vec3(0.0f, 1.0f, 0.0f);
        //camera.right_vector = glm::conjugate(camera.orientation) * glm::vec3(1.0f, 0.0f, 0.0f);

        // Create each of the orientations based on mouse offset and roll offset.
        //
        // This code snippet below locks the yaw to world coordinates.
        const glm::quat pitch = glm::angleAxis(glm::radians(yoffset), glm::vec3(1.0f, 0.0f, 0.0f));
//#define LOCK_YAW
#if defined(LOCK_YAW)
        const glm::quat yaw = glm::angleAxis(glm::radians(xoffset), camera.orientation * glm::vec3(0.0f, 1.0f, 0.0f));
#else
        const glm::quat yaw = glm::angleAxis(glm::radians(xoffset), glm::vec3(0.0f, 1.0f, 0.0f));
#endif
        const glm::quat roll = glm::angleAxis(glm::radians(camera.roll), glm::vec3(0.0f, 0.0f, 1.0f));

        // Update the camera orientation based on pitch, yaw and roll
        camera.orientation = (pitch * yaw * roll) * camera.orientation;

        // Create the view and projection matrices
        camera.vp.view = glm::mat4_cast(glm::quat(camera.orientation)) * glm::translate(glm::mat4(1.0f), -glm::vec3(camera.position));

        camera.roll = 0.0f;
#endif
    }

    void update_projection(perspective_camera& camera, uint32_t width, uint32_t height)
    {
        const auto aspect_ratio = static_cast<float>(width) /
            static_cast<float>(height);

        camera.vp.proj = glm::perspective(glm::radians(camera.fovy), aspect_ratio, camera.far_plane, camera.near_plane);

        // Required if using Vulkan (left-handed coordinate-system)
        camera.vp.proj[1][1] *= -1.0;

    }

    camera_frustum extract_frustum_planes(const glm::mat4& matrix)
    {
        camera_frustum frustum{};

        frustum.left.x = matrix[0].w + matrix[0].x;
        frustum.left.y = matrix[1].w + matrix[1].x;
        frustum.left.z = matrix[2].w + matrix[2].x;
        frustum.left.w = matrix[3].w + matrix[3].x;

        frustum.right.x = matrix[0].w - matrix[0].x;
        frustum.right.y = matrix[1].w - matrix[1].x;
        frustum.right.z = matrix[2].w - matrix[2].x;
        frustum.right.w = matrix[3].w - matrix[3].x;

        frustum.top.x = matrix[0].w - matrix[0].y;
        frustum.top.y = matrix[1].w - matrix[1].y;
        frustum.top.z = matrix[2].w - matrix[2].y;
        frustum.top.w = matrix[3].w - matrix[3].y;

        frustum.bottom.x = matrix[0].w + matrix[0].y;
        frustum.bottom.y = matrix[1].w + matrix[1].y;
        frustum.bottom.z = matrix[2].w + matrix[2].y;
        frustum.bottom.w = matrix[3].w + matrix[3].y;

        // todo: first part might not be required
        frustum.near.x = matrix[0].w + matrix[0].z;
        frustum.near.y = matrix[1].w + matrix[1].z;
        frustum.near.z = matrix[2].w + matrix[2].z;
        frustum.near.w = matrix[3].w + matrix[3].z;

        frustum.far.x = matrix[0].w - matrix[0].z;
        frustum.far.y = matrix[1].w - matrix[1].z;
        frustum.far.z = matrix[2].w - matrix[2].z;
        frustum.far.w = matrix[3].w - matrix[3].z;

        frustum.left = glm::normalize(frustum.left);
        frustum.right = glm::normalize(frustum.right);
        frustum.top = glm::normalize(frustum.top);
        frustum.bottom = glm::normalize(frustum.bottom);
        frustum.near = glm::normalize(frustum.near);
        frustum.far = glm::normalize(frustum.far);

        return frustum;
    }

}
