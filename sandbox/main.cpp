#include "../src/engine_platform.h"

float utc_time = 10.0f;

// This is the applications scale factor. This value gets applied to every
// object in the scene in order to maintain correct scaling values.
constexpr float scale_factor = 0.000005f;

// This is the applications speed factor relative to real world speeds.
// A value of 10 means that the simulation will run 10% faster than
// world speed.
constexpr int speed_factor = 10.0f;

// angular velocity of earth in degrees per second
// radians 0.0000729211533f
constexpr float angular_velocity = 0.004178074321326839639f * speed_factor;

constexpr float sun_radius = 696'340'000.0f * scale_factor;
constexpr float earth_radius = 6'378'137.0f * scale_factor;
constexpr float moon_radius = 1'737'400.0f * scale_factor;
constexpr float iss_altitude = 408'000.0f * scale_factor;

constexpr float iss_speed    = 0.07725304476742584f * speed_factor;

constexpr float sun_from_earth = 149'120'000'000.0f * scale_factor;
constexpr float moon_from_earth = 384'400'000.0f * scale_factor;

const const char* application_about = R"(
    3D Earth Visualizer is an application created and maintained by 
    Zakariya Oulhadj.

)";

/*
glm::vec3 sphere_translation(float radius, float latitude, float longitude)
{
    const float lat = glm::radians(latitude);
    const float lon = glm::radians(longitude);

    const float x = radius * glm::cos(lat) * glm::cos(lon);
    const float y = radius * glm::sin(lat);
    const float z = radius * glm::cos(lat) * sin(lon);

    return { x, y, z };
}
*/

glm::vec3 cartesian(float radius, float latitude, float longitude, float elevation = 0.0f)
{
    const float lat = glm::radians(latitude);
    const float lon = glm::radians(longitude);

    // If using WGS-84 coordinates
#if 0
    const float x = (radius + elevation) * glm::cos(lat) * glm::cos(lon);
    const float y = (radius + elevation) * glm::cos(lat) * glm::sin(lon);
    const float z = (radius + elevation) * glm::sin(lat);
#else
    const float x = (radius + elevation) * glm::cos(lat) * glm::cos(lon);
    const float y = (radius + elevation) * glm::sin(lat);
    const float z = (radius + elevation) * glm::cos(lat) * glm::sin(lon);
#endif

    return { x, y, z };
}

glm::vec2 geographic(float radius, const glm::vec3& position)
{
    // If using WGS-84 coordinates
#if 0
    const float latitude = glm::asin(position.z / radius);
    const float longitude = glm::atan(position.y, position.x);
#else
    const float latitude = glm::asin(position.y / radius);
    const float longitude = glm::atan(position.z, position.x);
#endif

    return { glm::degrees(latitude), glm::degrees(longitude) };
}

static void render_ui()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();


    static bool engine_options = false;
    static bool satellite_window = false;
    static bool documentation_window = false;
    static bool about_window = false;
	static bool demo_window = false;

    static bool wireframe = false;
    static bool triple_buffering = true;
    static bool vsync = true;
    static bool realtime = true;

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Engine")) {
            if (ImGui::MenuItem("Options"))
                engine_options = true;

            if (ImGui::MenuItem("Exit"))
                engine_stop();

			ImGui::EndMenu();
		}

        if (ImGui::BeginMenu("Tracking")) {
            if (ImGui::MenuItem("Satellites"))
                satellite_window = true;

            ImGui::EndMenu();
        }


		if (ImGui::BeginMenu("Simulation")) {
            ImGui::SliderFloat("Time (UTC)", &utc_time, 0.0f, 23.0f);
            ImGui::Checkbox("Realtime", &realtime);
            //if (!realtime)
            //    ImGui::SliderInt("Speed", &speed_factor, 1, 50, "%.2fx");


			ImGui::EndMenu();
		}


		if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Documentation"))
                documentation_window = true;
            
            if (ImGui::MenuItem("About"))
                about_window = true;
            
            if (ImGui::MenuItem("Show ImGui demo window"))
                demo_window = true;

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

    if (engine_options) {
        ImGui::Begin("Engine Options", &engine_options);

        ImGui::Checkbox("VSync", &vsync);
        ImGui::Checkbox("Triple Buffering", &triple_buffering);
        ImGui::Checkbox("Wireframe", &wireframe);
        ImGui::Text("Camera controls");


        ImGui::End();
    }

    if (satellite_window) {
		ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Satellites", &satellite_window, ImGuiWindowFlags_MenuBar))
		{

			// Left
			static int selected = 0;
			{
				ImGui::BeginChild("left pane", ImVec2(150, 0), true);
				for (int i = 0; i < 100; i++)
				{
					// FIXME: Good candidate to use ImGuiSelectableFlags_SelectOnNav
					char label[128];
					sprintf(label, "MyObject %d", i);
					if (ImGui::Selectable(label, selected == i))
						selected = i;
				}
				ImGui::EndChild();
			}
			ImGui::SameLine();

			// Right
			{
				ImGui::BeginGroup();
				ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
				ImGui::Text("MyObject: %d", selected);
				ImGui::Separator();
				if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
				{
					if (ImGui::BeginTabItem("Description"))
					{
						ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Details"))
					{
						ImGui::Text("ID: 00001");
                        ImGui::Text("Longitude: 51 30' 35.5140'\"' N");
                        ImGui::Text("Latitude:  0 7' 5.1312'\"' W");
                        ImGui::Text("Elevation: 324km");

						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::EndChild();

				if (ImGui::Button("Track")) {}
				ImGui::EndGroup();
			}
		}
		ImGui::End();
    }

    if (documentation_window) {
		ImGui::Begin("Documentation", &documentation_window);

		ImGui::End();
    }

	if (about_window) {
		ImGui::Begin("About", &about_window);
        ImGui::Text(application_about);
		ImGui::End();
	}


	if (demo_window)
		ImGui::ShowDemoWindow(&demo_window);


	ImGui::Render();
}


struct engine_scene
{
    glm::vec3 camera_pos;
    glm::vec3 sun_pos;
    glm::vec3 sun_color;
};

int main()
{
    engine_start("3D Earth Satellite Visualizer");

    const vertex_buffer* sphere = engine_load_model("assets/sphere_hp.obj");
    const vertex_buffer* cube   = engine_load_model("assets/iss.obj"); // This takes quite a long time to load

    const texture_buffer* sun_texture   = engine_load_texture("assets/textures/sun.jpg");
    const texture_buffer* earth_texture = engine_load_texture("assets/textures/earth.jpg");
    const texture_buffer* moon_texture  = engine_load_texture("assets/textures/moon.jpg");

    entity* sun_entity = engine_create_entity(sphere, sun_texture);
    entity* earth_entity = engine_create_entity(sphere, earth_texture);
    entity* moon_entity = engine_create_entity(sphere, moon_texture);

    entity* test_entity = engine_create_entity(cube, sun_texture);


    engine_scene scene;




    glm::vec3 london = cartesian(earth_radius, 51.5072, 0.1276, 2.0f);
    glm::vec2 london2 = geographic(earth_radius, london);

    printf("%s\n", glm::to_string(london).c_str());
    printf("%s\n", glm::to_string(london2).c_str());


    while (engine_running()) {
        if (engine_is_key_down(GLFW_KEY_ESCAPE)) engine_stop();


        // Camera movement
        if (engine_is_key_down(87))  engine_move_forwards();
        if (engine_is_key_down(83))  engine_move_backwards();
        if (engine_is_key_down(65))  engine_move_left();
        if (engine_is_key_down(68))  engine_move_right();
        if (engine_is_key_down(32))  engine_move_up();
        if (engine_is_key_down(341)) engine_move_down();
        if (engine_is_key_down(81))  engine_roll_left();
        if (engine_is_key_down(69))  engine_roll_right();

        // Update entities
        const float time = engine_uptime();
        const float earth_speed = angular_velocity * (time * speed_factor);

        engine_translate_entity(sun_entity, 0.0f, 0.0f, sun_from_earth);
        engine_scale_entity(sun_entity, sun_radius);

        engine_translate_entity(earth_entity, 0.0f, 0.0f, 0.0f);
        engine_scale_entity(earth_entity, earth_radius);
        engine_rotate_entity(earth_entity, earth_speed, 0.0f, 1.0f, 0.0f);


        engine_translate_entity(moon_entity, -moon_from_earth, 0.0f, -moon_from_earth);
        engine_scale_entity(moon_entity, moon_radius);


        glm::vec3 position = cartesian(earth_radius, 51.5072, 0.1276, 5.0f);
        engine_translate_entity(test_entity, position.x, position.y, position.z);
        engine_scale_entity(test_entity, 0.02f);

        // Rendering
        engine_render(sun_entity);
        engine_render(earth_entity);
        engine_render(moon_entity);
        engine_render(test_entity);


        render_ui();


        engine_render();

    }

    engine_exit();

    return 0;
}
