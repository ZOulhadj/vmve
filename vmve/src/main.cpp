#include <engine.h>

#include "variables.hpp"
//#include "security.hpp"
//#include "vmve.hpp"
//#include "ui.hpp"



static bool inViewport = false;


int main()
{
    Engine* engine = InitializeEngine();

    CreateCamera(engine, 45.0f, 20.0f);

    while (UpdateEngine(engine))
    {
        // only update input and camera when in the viewport
        if (inViewport)
        {

        }


        // Render shadow pass
        // Render deferred pass
        // Render Ui pass


        EnginePresent(engine);
    }

    TerminateEngine(engine);

    return 0;
}