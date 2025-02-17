#if defined(_DEBUG) || defined(DEBUG)
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#endif

#include <Windows.h>

#include "App.h"

#include "ImageProcessingHelperGPU.h"

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR pCmdLine,
    _In_ int nCmdShow
)
{
    App app(hInstance, nCmdShow);

    return app.Run();
}