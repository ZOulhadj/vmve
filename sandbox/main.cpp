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

const char* application_about = R"(
    3D Earth Visualizer is an application created and maintained by 
    Zakariya Oulhadj.

)";


static glm::vec3 sun_pos = glm::vec3(0.0f);
static glm::vec3 earth_pos = glm::vec3(0.0f);
static glm::vec3 moon_pos = glm::vec3(0.0f);

static glm::vec3 sun_color = glm::vec3(1.0f);

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


glm::vec2 world_to_screen(const glm::vec3& position, const glm::vec2& offset = glm::vec2(0.0f))
{
    const glm::mat4 proj = get_camera_projection();
    const glm::mat4 view = get_camera_view();
    const glm::vec2 win_size = get_window_size();

    const glm::vec4 clip_pos         = proj * (view * glm::vec4(position, 1.0));
    const glm::vec3 ndc_space_pos    = glm::vec3(clip_pos.x / clip_pos.w, clip_pos.y / clip_pos.w, clip_pos.z / clip_pos.w);
    const glm::vec2 window_space_pos = ((ndc_space_pos.xy() + 1.0f) / 2.0f) * win_size + offset;

    return window_space_pos;
}


static void RenderGUI()
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
    static bool overlay = false;

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

        ImGui::Checkbox("Entity overlay", &overlay);

        ImGui::SliderFloat3("Sun color", glm::value_ptr(sun_color), 0.0f, 1.0f);

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

    if (overlay) {
        // get screen space from world space
        glm::vec2 sun_screen_pos = world_to_screen(sun_pos);
        glm::vec2 earth_screen_pos = world_to_screen(earth_pos);
        glm::vec2 moon_screen_pos = world_to_screen(moon_pos);

        static bool o = true;

        const ImGuiWindowFlags flags = ImGuiWindowFlags_None |
            ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize;


        ImGui::SetNextWindowPos(ImVec2(sun_screen_pos.x, sun_screen_pos.y));
        ImGui::Begin("text0", &o, flags);
        ImGui::Text("Sun");
        ImGui::Text("%.2f, %.2f, %.2f", sun_pos.x, sun_pos.y, sun_pos.z);
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(earth_screen_pos.x, earth_screen_pos.y));
        ImGui::Begin("text1", &o, flags);
        ImGui::Text("Earth");
        ImGui::Text("%.2f, %.2f, %.2f", earth_pos.x, earth_pos.y, earth_pos.z);
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(moon_screen_pos.x, moon_screen_pos.y));
        ImGui::Begin("text2", &o, flags);
        ImGui::Text("Moon");
        ImGui::Text("%.2f, %.2f, %.2f", moon_pos.x, moon_pos.y, moon_pos.z);
        ImGui::End();
    }
    

	ImGui::Render();
}

int main()
{
    StartEngine("3D Earth Satellite Visualizer");

    const VertexBuffer* sphere = EngineLoadModel("assets/sphere.obj");

    const TextureBuffer* sun_texture   = EngineLoadTexture("assets/textures/sun.jpg");
    const TextureBuffer* earth_texture = EngineLoadTexture("assets/textures/earth.jpg");
    const TextureBuffer* moon_texture  = EngineLoadTexture("assets/textures/moon.jpg");

    Entity* sun_entity = EngineCreateEntity(sphere, sun_texture);
    Entity* earth_entity = EngineCreateEntity(sphere, earth_texture);
    Entity* moon_entity = EngineCreateEntity(sphere, moon_texture);


    while (IsEngineRunning()) {
        if (EngineIsKeyDown(GLFW_KEY_ESCAPE)) engine_stop();


        // Camera movement
        if (EngineIsKeyDown(87))  engine_move_forwards();
        if (EngineIsKeyDown(83))  engine_move_backwards();
        if (EngineIsKeyDown(65))  engine_move_left();
        if (EngineIsKeyDown(68))  engine_move_right();
        if (EngineIsKeyDown(32))  engine_move_up();
        if (EngineIsKeyDown(341)) engine_move_down();
        if (EngineIsKeyDown(81))  engine_roll_left();
        if (EngineIsKeyDown(69))  engine_roll_right();

        // Update entities
        const float time = EngineGetUptime();
        const float earth_speed = angular_velocity * (time * speed_factor);

        sun_pos = glm::vec3(glm::sin(glm::radians(-time)) * sun_from_earth, 0.0f, glm::cos(glm::radians(-time)) * sun_from_earth);
        earth_pos = glm::vec3(0.0f, 0.0f, 0.0f);
        moon_pos = glm::vec3(glm::sin(glm::radians(time)) * moon_from_earth, 0.0f, glm::cos(glm::radians(time)) * moon_from_earth);

        EngineTranslateEntity(sun_entity, sun_pos.x, sun_pos.y, sun_pos.z);
        EngineScaleEntity(sun_entity, sun_radius);
        EngineRotateEntity(sun_entity, time, 0.0f, 1.0f, 0.0f);

        EngineTranslateEntity(earth_entity, earth_pos.x, earth_pos.y, earth_pos.z);
        EngineScaleEntity(earth_entity, earth_radius);
        EngineRotateEntity(earth_entity, earth_speed, 0.0f, 1.0f, 0.0f);


        EngineTranslateEntity(moon_entity, moon_pos.x, moon_pos.y, moon_pos.z);
        EngineScaleEntity(moon_entity, moon_radius);
        EngineRotateEntity(moon_entity, time, 0.0f, 1.0f, 0.0f);

        // Rendering
        EngineRender(sun_entity);
        EngineRender(earth_entity);
        EngineRender(moon_entity);

        RenderGUI();

        EngineScene scene{};
        scene.camera_position = get_camera_position();
        scene.sun_position    = sun_pos;
        scene.sun_color       = sun_color;

        EngineRender(scene);

    }

    StopEngine();

    return 0;
}
