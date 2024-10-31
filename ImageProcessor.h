#pragma once

#include <cassert>
#include <vector>
#include <cstdint>

#include <d2d1.h>
#pragma comment(lib, "d2d1.lib")

#include "Debug.h"
#include "COMHelper.h"

enum
{
	TABLE_SIZE = 256
};

struct FrequencyTable
{
	uint32_t redTable[TABLE_SIZE];
	uint32_t redTotal;

	uint32_t greenTable[TABLE_SIZE];
	uint32_t greenTotal;

	uint32_t blueTable[TABLE_SIZE];
	uint32_t blueTotal;
};

union Pixel
{
	struct rgba
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} rgba;

	uint32_t pixel;
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

	ID2D1Bitmap* CreateBitmapHeap(ID2D1HwndRenderTarget& renderTarget) const;
	void DrawImage(ID2D1HwndRenderTarget& renderTarget) const;
	void DrawImageWithHistogram(ID2D1HwndRenderTarget& renderTarget) const;

private:
	std::vector<Pixel> mPixels;

	int mImageWidth;
	int mImageHeight;
	int mImageChannelCount;

	FrequencyTable mFrequencyTable;
	std::vector<float> mPMF;

private:
	D2D1_RECT_F getD2DRect(const HWND renderTarget) const;

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
