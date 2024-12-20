#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <ctime>
#include <immintrin.h>

#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")

#include "Debug.h"
#include "COMHelper.h"
#include "TypeDef.h"
#include "MathHelper.h"
#include "Image.h"

#include "ProcessingHelperCPU.h"
#include "ProcessingHelperGPU.h"

enum class EDrawMode
{
	DEFAULT,
	HISTOGRAM,
	EQUALIZATION,
	MATCHING, 
	GAMMA, 
	COUNT
};

class ImageProcessor final
{
public:
	ImageProcessor(const char* path);
	~ImageProcessor() = default;
	ImageProcessor(const ImageProcessor& other) = default;
	ImageProcessor& operator=(const ImageProcessor& other) = default;

	void SetDrawMode(const EDrawMode mode);

	void DrawImage(ID2D1HwndRenderTarget& renderTarget);

private:
	Image mImage;

	EDrawMode mDrawMode;

private:
	ID2D1Bitmap* createRenderedBitmapHeap(ID2D1HwndRenderTarget& renderTarget) const;
	D2D1_RECT_F getD2DRect(const HWND renderTarget) const;

	void drawImageDefault(ID2D1HwndRenderTarget& renderTarget) const;
	void drawImageHistogram(ID2D1HwndRenderTarget& renderTarget) const;
	void drawImageEqualization(ID2D1HwndRenderTarget& renderTarget);
	void drawImageMatching(ID2D1HwndRenderTarget& renderTarget);
	void drawImageGamma(ID2D1HwndRenderTarget& renderTarget);
};
