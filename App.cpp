#include "App.h"

static const TCHAR* const CLASS_NAME = TEXT("ImageProcessingPractice");

App::App(const HINSTANCE hInstance, const int nCmdShow)
    : mhWnd(nullptr)
    , mhInstance(nullptr)
    , mpDevice(nullptr)
    , mpDeviceContext(nullptr)
    , mpSwapChain(nullptr)
    , mpRenderTargetView(nullptr)
    , mpVertexShader(nullptr)
    , mpVertexBuffer(nullptr)
    , mpPixelShader(nullptr)
    , mpSampler(nullptr)
    , mImageProcessor()
    , mpImageGPU(nullptr)
    , mpImageGPUView(nullptr)
    , mUIEventFlags(0)
{
    ASSERT(hInstance != nullptr);
    ASSERT(nCmdShow >= 0);

    HRESULT hr = S_OK;
    char msg[EDebugConstant::DEFAULT_BUFFER_SIZE];

    // winapi
    {
        mhInstance = hInstance;

        WNDCLASSEX wndClass;
        ZeroMemory(&wndClass, sizeof(wndClass));

        wndClass.cbSize = sizeof(wndClass);
        wndClass.lpfnWndProc = App::wndProc;
        wndClass.lpszClassName = CLASS_NAME;
        wndClass.hInstance = hInstance;
        wndClass.hCursor = (HCURSOR)LoadCursor(nullptr, IDC_ARROW);
        wndClass.cbWndExtra = sizeof(this);

        RegisterClassEx(&wndClass);

        GetHResultDataFromWin32(hr, msg);
        ASSERT(SUCCEEDED(hr), msg);

        mhWnd = CreateWindowEx(
            0,
            CLASS_NAME,
            CLASS_NAME,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            nullptr,
            nullptr,
            hInstance,
            this
        );

        // GetHResultDataFromWin32(hr, msg);
        // ASSERT(SUCCEEDED(hr), msg);

        SetWindowLongPtr(mhWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        // GetHResultDataFromWin32(hr, msg);
        // ASSERT(SUCCEEDED(hr), msg);

        FileDialog::CreateInstance(mhWnd);
    }

    // d3d11
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG) || defined(DEBUG)
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        RECT windowRect;
        GetClientRect(mhWnd, &windowRect);

        // GetHResultDataFromWin32(hr, msg);
        // ASSERT(SUCCEEDED(hr), msg);

        DXGI_SWAP_CHAIN_DESC swapChainDesc;
        ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

        swapChainDesc.BufferCount = 1;
        swapChainDesc.BufferDesc.Width = windowRect.right;
        swapChainDesc.BufferDesc.Height = windowRect.bottom;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 144;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = mhWnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = true;

        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &swapChainDesc,
            &mpSwapChain,
            &mpDevice,
            nullptr,
            &mpDeviceContext
        );

        GetErrorDescription(hr, msg);
        ASSERT(SUCCEEDED(hr), msg);

        ID3DBlob* pVSBlob = nullptr;
        ID3DBlob* pPSBlob = nullptr;
        ID3DBlob* pErrorBlob = nullptr;
        {
            UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DEBUG)
            compileFlags |= D3DCOMPILE_DEBUG;
            compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
            // vs
            hr = D3DCompileFromFile(
                TEXT("VS.hlsl"),
                nullptr,
                nullptr,
                "main",
                "vs_5_0",
                compileFlags,
                0,
                &pVSBlob,
                &pErrorBlob
            );
            ASSERT(SUCCEEDED(hr), pErrorBlob != nullptr ? pErrorBlob->GetBufferPointer() : "");

            hr = mpDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &mpVertexShader);

            GetErrorDescription(hr, msg);
            ASSERT(SUCCEEDED(hr), msg);

            mpDeviceContext->VSSetShader(mpVertexShader, nullptr, 0);

            // ps
            hr = D3DCompileFromFile(
                TEXT("PS.hlsl"),
                nullptr,
                nullptr,
                "main",
                "ps_5_0",
                compileFlags,
                0,
                &pPSBlob,
                &pErrorBlob
            );
            ASSERT(SUCCEEDED(hr), pErrorBlob != nullptr ? pErrorBlob->GetBufferPointer() : "");

            hr = mpDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &mpPixelShader);

            GetErrorDescription(hr, msg);
            ASSERT(SUCCEEDED(hr), msg);

            D3D11_SAMPLER_DESC samplerDesc;
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.MipLODBias = 0;
            samplerDesc.MaxAnisotropy = 0;
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

            hr = mpDevice->CreateSamplerState(&samplerDesc, &mpSampler);

            GetErrorDescription(hr, msg);
            ASSERT(SUCCEEDED(hr), msg);

            mpDeviceContext->PSSetShader(mpPixelShader, nullptr, 0);
            mpDeviceContext->PSSetSamplers(0, 1, &mpSampler);

            // IA
            D3D11_INPUT_ELEMENT_DESC layoutArr[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
            };

            ID3D11InputLayout* pVertexLayout = nullptr;
            {
                hr = mpDevice->CreateInputLayout(
                    layoutArr,
                    ARRAYSIZE(layoutArr),
                    pVSBlob->GetBufferPointer(),
                    pVSBlob->GetBufferSize(),
                    &pVertexLayout
                );

                mpDeviceContext->IASetInputLayout(pVertexLayout);

                GetErrorDescription(hr, msg);
                ASSERT(SUCCEEDED(hr), msg);
            }
            SafeRelease(pVertexLayout);

            float vertices[] = {
                -1.f, -1.f, // x, y
                -1.f, 1.f,
                1.f, -1.f,
                1.f, 1.f
            };

            const UINT stride = sizeof(float) * 2;
            const UINT offset = 0;

            D3D11_BUFFER_DESC bufferDesc;
            ZeroMemory(&bufferDesc, sizeof(bufferDesc));

            bufferDesc.ByteWidth = sizeof(vertices);
            bufferDesc.Usage = D3D11_USAGE_DEFAULT;
            bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bufferDesc.CPUAccessFlags = 0;
            bufferDesc.MiscFlags = 0;
            bufferDesc.StructureByteStride = stride;

            D3D11_SUBRESOURCE_DATA vertexData;
            vertexData.pSysMem = vertices;
            vertexData.SysMemPitch = 0;
            vertexData.SysMemSlicePitch = 0;

            hr = mpDevice->CreateBuffer(
                &bufferDesc,
                &vertexData,
                &mpVertexBuffer
            );

            GetErrorDescription(hr, msg);
            ASSERT(SUCCEEDED(hr), msg);

            mpDeviceContext->IASetVertexBuffers(0, 1, &mpVertexBuffer, &stride, &offset);
            mpDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        }
        SafeRelease(pVSBlob);
        SafeRelease(pPSBlob);

        // RS, OM
        onResize(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
    }

    // imgui
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplWin32_Init(mhWnd);
        ImGui_ImplDX11_Init(mpDevice, mpDeviceContext);
    }

    // image
    {
        loadImage("default.png");
    }

    ShowWindow(mhWnd, nCmdShow);

    GetHResultDataFromWin32(hr, msg);
    ASSERT(SUCCEEDED(hr), msg);
}

App::~App()
{
    // imgui
    {
        ImGui_ImplWin32_Shutdown();
        ImGui_ImplDX11_Shutdown();
        ImGui::DestroyContext();
    }

    //d3d
    {
        SafeRelease(mpImageGPUView);
        SafeRelease(mpImageGPU);
        SafeRelease(mpSampler);
        SafeRelease(mpPixelShader);
        SafeRelease(mpVertexBuffer);
        SafeRelease(mpVertexShader);
        SafeRelease(mpRenderTargetView);
        SafeRelease(mpSwapChain);
        SafeRelease(mpDeviceContext);
        SafeRelease(mpDevice);
    }

    // win32
    {
        FileDialog::DeleteInstance();

        UnregisterClass(CLASS_NAME, mhInstance);
    }
}

int App::Run()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        mImageProcessor.Update();
    }

    return (int)msg.wParam;
}

LRESULT App::wndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
    App* const pThis = reinterpret_cast<App*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (pThis == nullptr)
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return pThis->handleWndCallback(hWnd, message, wParam, lParam);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT App::handleWndCallback(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
    if (mUIEventFlags != 0)
    {
        if (isOnUIEvent(EUIEventMask::FILE_OPEN))
        {
            mUIEventFlags &= ~EUIEventMask::FILE_OPEN;

            char buffer[EFileDialogConstant::DEFAULT_PATH_LEN];

            FileDialog& fileDialog = *FileDialog::GetInstance();
            if (fileDialog.TryOpenFileDialog(buffer, EFileDialogConstant::DEFAULT_PATH_LEN))
            {
                loadImage(buffer);
            }
        }

        ASSERT(mUIEventFlags == 0);

        return 0;
    }

    switch (message)
    {
    case WM_SIZE:
        onResize(LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_PAINT:
        onDraw();
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        {
            return 0;
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

void App::onDraw()
{
    ASSERT(mpDeviceContext != nullptr);
    ASSERT(mpSwapChain != nullptr);

    const Image& imageToDraw = mImageProcessor.GetProcessedImage();

    // output
    {
        D3D11_MAPPED_SUBRESOURCE mappedEntity;
        ZeroMemory(&mappedEntity, sizeof(mappedEntity));

        mpDeviceContext->Map(mpImageGPU, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedEntity);
        {
            uint8_t* pOutput = static_cast<uint8_t*>(mappedEntity.pData);
            const Pixel* pRaw = imageToDraw.pRawPixels;

            for (int y = 0; y < imageToDraw.Height; ++y)
            {
                memcpy(pOutput, pRaw, sizeof(Pixel) * imageToDraw.Width);

                pOutput += mappedEntity.RowPitch;
                pRaw += imageToDraw.Width;
            }
        }
        mpDeviceContext->Unmap(mpImageGPU, 0);

        mpDeviceContext->Draw(4, 0);
    }

    // ui
    {
        ImGui_ImplWin32_NewFrame();
        ImGui_ImplDX11_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::BeginMainMenuBar();
            {
                if (ImGui::MenuItem("Open", "Ctrl+O"))
                {
                    mUIEventFlags |= EUIEventMask::FILE_OPEN;
                }
            }
            ImGui::EndMainMenuBar();

            mImageProcessor.DrawControlPanel();
        }
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    mpSwapChain->Present(1, 0);
}

void App::onResize(const int width, const int height)
{
    if (mpSwapChain == nullptr)
    {
        return;
    }

    char msg[EDebugConstant::DEFAULT_BUFFER_SIZE];

    // RS
    D3D11_VIEWPORT viewport;
    viewport.Width = static_cast<FLOAT>(width);
    viewport.Height = static_cast<FLOAT>(height);
    viewport.MinDepth = 0.f;
    viewport.MaxDepth = 1.f;
    viewport.TopLeftX = 0.f;
    viewport.TopLeftY = 0.f;

    ASSERT(mpDeviceContext != nullptr);

    mpDeviceContext->RSSetViewports(1, &viewport);

    // OM
    mpDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    SafeRelease(mpRenderTargetView);

    HRESULT hr = mpSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

    GetErrorDescription(hr, msg);
    ASSERT(SUCCEEDED(hr), msg);

    ID3D11Texture2D* pBackBuffer = nullptr;
    {
        hr = mpSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));

        GetErrorDescription(hr, msg);
        ASSERT(SUCCEEDED(hr), msg);

        hr = mpDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mpRenderTargetView);

        GetErrorDescription(hr, msg);
        ASSERT(SUCCEEDED(hr), msg);
    }
    SafeRelease(pBackBuffer);

    mpDeviceContext->OMSetRenderTargets(1, &mpRenderTargetView, nullptr);
}

void App::loadImage(const char* path)
{
    Image newImage(path);
    {
        char msg[EDebugConstant::DEFAULT_BUFFER_SIZE];

        SafeRelease(mpImageGPU);
        SafeRelease(mpImageGPUView);

        D3D11_TEXTURE2D_DESC imageDesc;
        imageDesc.Width = newImage.Width;
        imageDesc.Height = newImage.Height;
        imageDesc.MipLevels = 1;
        imageDesc.ArraySize = 1;
        imageDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        imageDesc.SampleDesc.Count = 1;
        imageDesc.SampleDesc.Quality = 0;
        imageDesc.Usage = D3D11_USAGE_DYNAMIC;
        imageDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        imageDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        imageDesc.MiscFlags = 0;

        HRESULT hr = mpDevice->CreateTexture2D(&imageDesc, nullptr, &mpImageGPU);

        GetErrorDescription(hr, msg);
        ASSERT(SUCCEEDED(hr), msg);

        D3D11_SHADER_RESOURCE_VIEW_DESC imageViewDesc;
        ZeroMemory(&imageViewDesc, sizeof(imageViewDesc));

        imageViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        imageViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        imageViewDesc.Texture2D.MipLevels = 1;

        mpDevice->CreateShaderResourceView(mpImageGPU, nullptr, &mpImageGPUView);

        GetErrorDescription(hr, msg);
        ASSERT(SUCCEEDED(hr), msg);

        mpDeviceContext->PSSetShaderResources(0, 1, &mpImageGPUView);
    }
    mImageProcessor.RegisterImage(std::move(newImage));
}

