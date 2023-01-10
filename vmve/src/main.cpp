#include <engine.h>

#include "vmve.hpp"
#include "security.hpp"
//#include "ui.hpp"

const char* buildVersion = "0.0.1";
const char* buildDate = "05/05/2023";
const char* authorName = "Zakariya Oulhadj";
const char* authorEmail = "example@gmail.com";
const char* applicationAbout = R"(
    This is a 3D model viewer and exporter designed for the purpose of graphics
    debugging and testing.
)";


int main()
{
    // TODO: Icon needs to be bundled with the executable and this needs to be
    // the case for both the windows icon and glfw window icon
    EngineInfo info{};
    info.iconPath = "icon.png";
    info.windowWidth = 1920;
    info.windowHeight = 1080;

    Engine* engine = InitializeEngine(info);

    CreateCamera(engine, 45.0f, 20.0f);

    while (UpdateEngine(engine))
    {

        EnginePresent(engine);
    }

    TerminateEngine(engine);

    return 0;
}