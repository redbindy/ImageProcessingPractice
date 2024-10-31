#pragma once

#include <Windows.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")

#include "Debug.h"
#include "COMHelper.h"
#include "ImageProcessor.h"

class App final
{
public:
	App();
	~App();
	App(const App& other) = delete;
	App& operator=(const App& other) = delete;

	HRESULT Initiallize();
	void Run() const;

private:
	HWND mHWnd;
	ID2D1Factory* mpD2DFactory;
	ID2D1HwndRenderTarget* mpD2DHWndRenderTarget;

	ImageProcessor mImageProcessor;

private:
	static LRESULT wndProc(
		const HWND hWnd,
		const UINT message,
		const WPARAM wParam,
		const LPARAM lParam
	);

	HRESULT createDeviceIndependentResources();
	HRESULT createDeviceResources();

	void onRender();
	void onResize(const int width, const int height);
};