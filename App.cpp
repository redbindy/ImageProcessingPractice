#include "App.h"

App::App()
	: mHWnd(nullptr)
	, mpD2DFactory(nullptr)
	, mpD2DHWndRenderTarget(nullptr)
	, mImageProcessor("wallpaper_1920_A.jpg")
{
}

App::~App()
{
	SafeRelease(mpD2DHWndRenderTarget);
	SafeRelease(mpD2DFactory);
}

HRESULT App::Initiallize()
{
	HRESULT hr = S_OK;

	const HINSTANCE hInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
	const TCHAR* CLASS_NAME = TEXT("ImageProcessingPractice");

	WNDCLASSEX wndClass;
	ZeroMemory(&wndClass, sizeof(wndClass));

	wndClass.cbSize = sizeof(wndClass);
	wndClass.lpfnWndProc = App::wndProc;
	wndClass.lpszClassName = CLASS_NAME;
	wndClass.hInstance = hInstance;
	wndClass.hCursor = (HCURSOR)LoadCursor(nullptr, IDC_ARROW);
	wndClass.cbWndExtra = sizeof(LONG_PTR);

	RegisterClassEx(&wndClass);

	hr = HRESULT_FROM_WIN32(GetLastError());
	if (FAILED(hr))
	{
		ASSERT(false);
		PrintErrorMessage("RegisterClassEx", hr);

		return hr;
	}

	mHWnd = CreateWindowEx(
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

#if false

	hr = HRESULT_FROM_WIN32(GetLastError());
	if (FAILED(hr))
	{
		ASSERT(false);
		PrintErrorMessage("CreateWindowEx", hr);

		return hr;
	}

#endif

	ShowWindow(mHWnd, SW_SHOWNORMAL);

	hr = HRESULT_FROM_WIN32(GetLastError());
	if (FAILED(hr))
	{
		ASSERT(false);
		PrintErrorMessage("ShowWindow", hr);

		return hr;
	}

	hr = createDeviceIndependentResources();
	if (FAILED(hr))
	{
		ASSERT(false);
		PrintErrorMessage("createDeviceIndependentResources", hr);

		return hr;
	}

	return hr;
}

void App::Run() const
{
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT App::wndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
	if (message != WM_CREATE)
	{
		App* pApp = (App*)(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		if (pApp == nullptr)
		{
			goto DEFAULT_FUNC;
		}

		switch (message)
		{
		case WM_SIZE:
			{
				const UINT width = LOWORD(lParam);
				const UINT height = HIWORD(wParam);
				pApp->onResize(width, height);
			}
			break;

		case WM_CHAR:
			switch (wParam)
			{
			case '1':
				pApp->mImageProcessor.SetDrawMode(EDrawMode::DEFAULT);
				break;

			case '2':
				pApp->mImageProcessor.SetDrawMode(EDrawMode::HISTOGRAM);
				break;

			case '3':
				pApp->mImageProcessor.SetDrawMode(EDrawMode::EQUALIZATION);
				break;

			case '4':
				pApp->mImageProcessor.SetDrawMode(EDrawMode::MATCHING);
				break;

			case '5':
				pApp->mImageProcessor.SetDrawMode(EDrawMode::GAMMA);
				break;

			default:
				break;
			}
			break;

		case WM_PAINT:
			pApp->onRender();
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		DEFAULT_FUNC:
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	else
	{
		CREATESTRUCT* pCS = (CREATESTRUCT*)(lParam);
		App* pApp = (App*)pCS->lpCreateParams;

		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)(pApp));

		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		ASSERT(SUCCEEDED(hr), "SetWindowPtr", hr);
	}

	return 0;
}

HRESULT App::createDeviceIndependentResources()
{
	HRESULT hr = S_OK;

	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &mpD2DFactory);
	if (FAILED(hr))
	{
		ASSERT(false);
		PrintErrorMessage("D2D1CreateFactory", hr);

		return hr;
	}

	return hr;
}

HRESULT App::createDeviceResources()
{
	HRESULT hr = S_OK;

	SafeRelease(mpD2DHWndRenderTarget);

	RECT rect;
	GetClientRect(mHWnd, &rect);

	hr = HRESULT_FROM_WIN32(GetLastError());

	D2D_SIZE_U size = D2D1::SizeU(
		rect.right - rect.left,
		rect.bottom - rect.top
	);

	hr = mpD2DFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(mHWnd, size),
		&mpD2DHWndRenderTarget
	);

	return hr;
}

void App::onRender()
{
	createDeviceResources();

	mpD2DHWndRenderTarget->BeginDraw();
	{
		mpD2DHWndRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		mImageProcessor.DrawImage(*mpD2DHWndRenderTarget);
	}
	mpD2DHWndRenderTarget->EndDraw();
}

void App::onResize(const UINT width, const UINT height)
{
	if (mpD2DHWndRenderTarget == nullptr)
	{
		return;
	}

	D2D1_SIZE_U size = { width, height };

	mpD2DHWndRenderTarget->Resize(size);
}
