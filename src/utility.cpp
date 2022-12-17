#include "utility.hpp"


std::string load_text_file(std::string_view path)
{
    std::ifstream file(path.data());
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}


void write_text_file(std::string_view path)
{
    std::ofstream file(path.data());
    file << "Hello, World\n";
    file << "This is another sentence\n";
}

double get_delta_time()
{


#define CHRONO 1
#if CHRONO
    static auto last_time = std::chrono::high_resolution_clock::now();
    const auto current_time = std::chrono::high_resolution_clock::now();
    using ts = std::chrono::duration<double, std::milli>;
    const double delta_time = std::chrono::duration_cast<ts>(current_time - last_time).count() / 1000.0f;
    last_time = current_time;
#else
    static double lastTime;
    double currentTime = glfwGetTime();
    double delta_time = currentTime - lastTime;
    lastTime = currentTime;
#endif

    return delta_time;
}

glm::vec2 world_to_screen(window_t* window,
                          camera_t& camera,
                          const glm::vec3& position,
                          const glm::vec2& offset)
{
    const glm::vec2 windowSize = glm::vec2(window->width, window->height);

    const glm::vec4 clip   = camera.viewProj.proj * camera.viewProj.view * glm::vec4(position, 1.0);
    const glm::vec3 ndc    = clip.xyz() / clip.w;
    const glm::vec2 screen = ((ndc.xy() + 1.0f) / 2.0f) * windowSize + offset;

    return screen;
}
