#include <Windows.h>

#include "App.h"
#include "ProcessingHelperGPU.h"

int main()
{
    App app;

    int result = app.Initiallize();
    if (result == S_OK)
    {
        app.Run();
    }

    return result;
}
