#include "camera.h"

static Frustum_Plane CreateFrustum(const glm::vec3& normal, const glm::vec3& point) {
    Frustum_Plane plane;
    plane.normal = glm::normalize(normal);
    plane.distance_from_origin = glm::dot(plane.normal, point);

    return plane;
}

Frustum create_camera_frustum(const Camera& cam) {
    // NOTE: The camera frustum can also be extracted from the MVP
    // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf    
    
    Frustum frustum{};


    constexpr float far = std::numeric_limits<float>::max();
    const float aspect_ratio = cam.width / cam.height;

    const float half_height = far * glm::tan(cam.fov * 0.5f);
    const float half_width = half_height * aspect_ratio;
    const glm::vec3 front_mult_far = far * cam.front_vector;

    frustum.near = CreateFrustum(cam.front_vector, cam.position + cam.near * cam.front_vector);
    frustum.far  = CreateFrustum(-cam.front_vector, cam.position + front_mult_far);
    frustum.right = CreateFrustum(glm::cross(cam.up_vector, front_mult_far + cam.right_vector * half_width), cam.position);
    frustum.left = CreateFrustum(glm::cross(front_mult_far - cam.right_vector * half_width, cam.up_vector), cam.position);
    frustum.top = CreateFrustum(glm::cross(cam.right_vector, front_mult_far - cam.up_vector * half_height), cam.position);
    frustum.bottom = CreateFrustum(glm::cross(front_mult_far + cam.up_vector * half_height, cam.right_vector), cam.position);


    return frustum;
}

static Camera create_camera(Camera_Type type, 
    Camera_Projection projection, 
    const glm::vec3& pos, 
    float speed,
    float fov) {
    Camera cam{};

    cam.position    = pos;
    cam.orientation = glm::quat(1, 0, 0, 0);
    cam.width       = 1280.0f;
    cam.height      = 720.0f;
    cam.speed       = speed;
    cam.view_speed  = 0.1f;
    cam.roll_speed  = 45.0f;
    cam.roll        = 0.0f;
    cam.fov         = fov;
    cam.near        = 0.1f;
    cam.far = 2000.0f;
    cam.type = type;

    if (cam.type == Camera_Type::first_person) {
        cam.viewProj.view = glm::mat4_cast(glm::quat(cam.orientation)) * 
                                           glm::translate(glm::mat4(1.0f), -glm::vec3(cam.position));
    } else if (cam.type == Camera_Type::look_at) {
        cam.viewProj.view = glm::lookAt(cam.position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }


    if (projection == Camera_Projection::perspective) {
        cam.viewProj.proj = glm::perspective(glm::radians(fov), (float)cam.width / cam.height, cam.near, cam.far);
        //cam.viewProj.proj = infinite_perspective(glm::radians(cam.fov), { cam.width, cam.height }, cam.near);
    } else if (projection == Camera_Projection::orthographic) {
        cam.viewProj.proj = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, cam.near, cam.far);
    }

    // Required if using Vulkan (left-handed coordinate-system)
    cam.viewProj.proj[1][1] *= -1.0;


    return cam;
}


Camera create_perspective_camera(Camera_Type type, const glm::vec3& position, float fov, float speed) {
    return create_camera(type, Camera_Projection::perspective, position, speed, fov);
}


Camera create_orthographic_camera(Camera_Type type, const glm::vec3& position, float speed) {
    // Note that an orthographic camera does not use FOV
    return create_camera(type, Camera_Projection::orthographic, position, speed, 0.0f);
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
//Camera CreateCamera(const glm::vec3& position, float fov, float speed)
//{
//    Camera cam{};
//    cam.position    = position;
//    cam.orientation = glm::quat(1, 0, 0, 0);
//    cam.width       = 1280.0f;
//    cam.height      = 720.0f;
//    cam.speed       = speed;
//    cam.view_speed  = 0.1f;
//    cam.roll_speed  = 45.0f;
//    cam.roll        = 0.0f;
//    cam.fov         = fov;
//    cam.near        = 0.1f;
//    cam.viewProj.view = glm::mat4_cast(glm::quat(cam.orientation)) * glm::translate(glm::mat4(1.0f), -glm::vec3(cam.position));
//    cam.viewProj.proj = infinite_perspective(glm::radians(cam.fov), { cam.width, cam.height }, cam.near);
//
//    // Required if using Vulkan (left-handed coordinate-system)
//    cam.viewProj.proj[1][1] *= -1.0;
//
//
//    cam.first_mouse = true;
//
//    return cam;
//}

void update_camera(Camera& camera, const glm::vec2& cursor_pos) {
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

    // Get the camera current direction vectors based on orientation
    camera.front_vector = glm::conjugate(camera.orientation) * glm::vec3(0.0f, 0.0f, 1.0f);
    camera.up_vector    = glm::conjugate(camera.orientation) * glm::vec3(0.0f, 1.0f, 0.0f);
    camera.right_vector = glm::conjugate(camera.orientation) * glm::vec3(1.0f, 0.0f, 0.0f);

    if (camera.type == Camera_Type::first_person) {
        // Create each of the orientations based on mouse offset and roll offset.
        //
        // This code snippet below locks the yaw to world coordinates.
        const glm::quat pitch = glm::angleAxis(glm::radians(yoffset), glm::vec3(1.0f, 0.0f, 0.0f));
#define LOCK_YAW
#if defined(LOCK_YAW)
        const glm::quat yaw = glm::angleAxis(glm::radians(xoffset), camera.orientation * glm::vec3(0.0f, 1.0f, 0.0f));
#else
        const glm::quat yaw = glm::angleAxis(glm::radians(xoffset), glm::vec3(0.0f, 1.0f, 0.0f));
#endif
        const glm::quat roll = glm::angleAxis(glm::radians(camera.roll), glm::vec3(0.0f, 0.0f, 1.0f));

        // Update the camera orientation based on pitch, yaw and roll
        camera.orientation = (pitch * yaw * roll) * camera.orientation;

        // Create the view and projection matrices
        camera.viewProj.view = glm::mat4_cast(glm::quat(camera.orientation)) * glm::translate(glm::mat4(1.0f), -glm::vec3(camera.position));
        //camera.viewProj.proj[1][1] = -1.0;

        // Build final camera transform matrix
        camera.roll = 0.0f;
    } else if (camera.type == Camera_Type::look_at) {
        camera.viewProj.view = glm::lookAt(camera.position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }
}


void update_projection(Camera& cam) {
    if (cam.projection == Camera_Projection::perspective) {
        cam.viewProj.proj = glm::perspective(glm::radians(cam.fov), (float)cam.width / cam.height, cam.near, cam.far);
    } else if (cam.projection == Camera_Projection::orthographic) {
        cam.viewProj.proj = glm::ortho(-100.0f, 100.0f, -100.0f, 100.0f, cam.near, cam.far);
    }

    // Required if using Vulkan (left-handed coordinate-system)
    cam.viewProj.proj[1][1] *= -1.0;
}

void update_projection(Camera& camera, uint32_t width, uint32_t height) {
    camera.width = width;
    camera.height = height;

    update_projection(camera);
}