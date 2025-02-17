#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler")

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include "Debug.h"
#include "ComHelper.h"

#include "FileDialog.h"

#include "Image.h"
#include "ImageProcessor.h"

class App final
{
public:
    using UIFlags = uint32_t;
    enum EUIEventMask
    {
        FILE_OPEN = 1
    };

public:
    App(const HINSTANCE hInstance, const int nCmdShow);
    ~App();
    App(const App& other) = delete;
    App(App&& other) = delete;
    App& operator=(const App& other) = delete;
    App& operator=(App&& other) = delete;

     int Run();

private:
    // win32
    HWND mhWnd;
    HINSTANCE mhInstance;

    // d3d
    ID3D11Device* mpDevice;
    ID3D11DeviceContext* mpDeviceContext;
    IDXGISwapChain* mpSwapChain;
    ID3D11RenderTargetView* mpRenderTargetView;

    ID3D11VertexShader* mpVertexShader;
    ID3D11Buffer* mpVertexBuffer;

    ID3D11PixelShader* mpPixelShader;
    ID3D11SamplerState* mpSampler;

    // image
    ImageProcessor mImageProcessor;

    ID3D11Texture2D* mpImageGPU;
    ID3D11ShaderResourceView* mpImageGPUView;

    // ui
    UIFlags mUIEventFlags;

private:
    static LRESULT CALLBACK wndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam);
    LRESULT CALLBACK handleWndCallback(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam);

    void onDraw();
    void onResize(const int width, const int height);

    inline bool isOnUIEvent(const EUIEventMask mask);

    void loadImage(const char* path);
};

inline bool App::isOnUIEvent(const EUIEventMask mask)
{
    return (mUIEventFlags & mask) != 0;
}
