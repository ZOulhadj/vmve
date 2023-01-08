#include <engine.h>

#include "variables.hpp"
//#include "security.hpp"
//#include "vmve.hpp"
//#include "ui.hpp"

#include <vector>
#include <string>
#include <filesystem>


int main()
{
    Engine* engine = InitializeEngine();

    CreateCamera(engine, 45.0f, 20.0f);


    while (UpdateEngine(engine))
    {

        EnginePresent(engine);
    }

    TerminateEngine(engine);

    return 0;
}