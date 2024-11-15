#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <ctime>

#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")

#include "Debug.h"
#include "COMHelper.h"
#include "TypeDef.h"

#include "ProcessingHelperCPU.h"
#include "ProcessingHelperGPU.h"

enum class EDrawMode
{
	DEFAULT,
	HISTOGRAM,
	EQUALIZATION,
	COUNT
};

class ImageProcessor final
{
public:
	ImageProcessor(const char* path);
	~ImageProcessor() = default;
	ImageProcessor(const ImageProcessor& other) = default;
	ImageProcessor& operator=(const ImageProcessor& other) = default;

	int GetImageWidth() const;
	int GetImageHeight() const;
	int GetImageComponentCount() const;

	void SetDrawMode(const EDrawMode mode);

	void DrawImage(ID2D1HwndRenderTarget& renderTarget);

private:
	std::vector<Pixel> mPixels;

	int mImageWidth;
	int mImageHeight;
	int mImageChannelCount;

	EDrawMode mDrawMode;

	FrequencyTable mFrequencyTable;

private:
	ID2D1Bitmap* createRenderedBitmapHeap(ID2D1HwndRenderTarget& renderTarget) const;
	D2D1_RECT_F getD2DRect(const HWND renderTarget) const;
	void drawImageDefault(ID2D1HwndRenderTarget& renderTarget) const;
	void drawImageHistogram(ID2D1HwndRenderTarget& renderTarget) const;
	void drawImageEqualization(ID2D1HwndRenderTarget& renderTarget);

	inline float remapValue(
		const float value,
		const float currMin,
		const float currMax,
		const float newMin,
		const float newMax
	) const;

	inline int toIndex(const int x, const int y, const int width) const;
};

inline float ImageProcessor::remapValue(
	const float value,
	const float currMin,
	const float currMax,
	const float newMin,
	const float newMax
) const
{
	return newMin + ((value - currMin) / (currMax - currMin)) * (newMax - newMin);
}

inline int ImageProcessor::toIndex(const int x, const int y, const int width) const
{
	return y * width + x;
}
