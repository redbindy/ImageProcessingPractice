#include "ImageProcessor.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

ImageProcessor::ImageProcessor(const char* path)
	: mImageWidth(0)
	, mImageHeight(0)
	, mImageChannelCount(0)
	, mFrequencyTable{ 0, }
{
	unsigned char* pPixelData = stbi_load(
		path,
		&mImageWidth,
		&mImageHeight,
		&mImageChannelCount,
		0
	);
	ASSERT(mImageChannelCount >= 3, "Image channel should be 3 >=");
	{
		mPixels.reserve(mImageWidth * mImageHeight);

		for (int y = 0; y < mImageHeight; ++y)
		{
			for (int x = 0; x < mImageWidth; ++x)
			{
				const int index = toIndex(x, y, mImageWidth) * mImageChannelCount;

				Pixel pixel;

				pixel.rgba.r = pPixelData[index];
				pixel.rgba.g = pPixelData[index + 1];
				pixel.rgba.b = pPixelData[index + 2];
				pixel.rgba.a = UINT8_MAX;

				++mFrequencyTable.redTable[pixel.rgba.r];
				++mFrequencyTable.redTotal;

				++mFrequencyTable.greenTable[pixel.rgba.g];
				++mFrequencyTable.greenTotal;

				++mFrequencyTable.blueTable[pixel.rgba.b];
				++mFrequencyTable.blueTotal;

				mPixels.push_back(pixel);
			}
		}
	}
	stbi_image_free(pPixelData);
}

int ImageProcessor::GetImageWidth() const
{
	return mImageWidth;
}

int ImageProcessor::GetImageHeight() const
{
	return mImageHeight;
}

int ImageProcessor::GetImageComponentCount() const
{
	return mImageChannelCount;
}

ID2D1Bitmap* ImageProcessor::CreateBitmapHeap(ID2D1HwndRenderTarget& renderTarget) const
{
	ID2D1Bitmap* pBitmap = nullptr;

	const D2D1_SIZE_U size = {
		static_cast<UINT32>(mImageWidth),
		static_cast<UINT32>(mImageHeight)
	};

	const D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		D2D1_ALPHA_MODE_IGNORE
	);

	const D2D1_BITMAP_PROPERTIES bitmapProperties = D2D1::BitmapProperties(pixelFormat);

	const HRESULT hr = renderTarget.CreateBitmap(
		size,
		bitmapProperties,
		&pBitmap
	);
	ASSERT(SUCCEEDED(hr), "CreateBitmap", hr);

	D2D1_RECT_U box;
	box.left = 0;
	box.top = 0;
	box.right = size.width;
	box.bottom = size.height;

	assert(pBitmap != nullptr);
	pBitmap->CopyFromMemory(
		&box,
		mPixels.data(),
		sizeof(Pixel) * size.width
	);

	return pBitmap;
}

D2D1_RECT_F ImageProcessor::getD2DRect(const HWND hWnd) const
{
	RECT rect;
	GetClientRect(hWnd, &rect);

	D2D1_RECT_F retRect;
	retRect.left = static_cast<FLOAT>(rect.left);
	retRect.top = static_cast<FLOAT>(rect.top);
	retRect.right = static_cast<FLOAT>(rect.right);
	retRect.bottom = static_cast<FLOAT>(rect.bottom);

	return retRect;
}

void ImageProcessor::DrawImage(ID2D1HwndRenderTarget& renderTarget) const
{
	ID2D1Bitmap* pBitmap = CreateBitmapHeap(renderTarget);
	{
		const D2D1_RECT_F imageRect = getD2DRect(renderTarget.GetHwnd());

		renderTarget.DrawBitmap(pBitmap, imageRect);
	}
	pBitmap->Release();
}

void ImageProcessor::DrawImageWithHistogram(ID2D1HwndRenderTarget& renderTarget) const
{
	ID2D1Bitmap* pBitmap = CreateBitmapHeap(renderTarget);
	{
		D2D1_RECT_F imageRect = getD2DRect(renderTarget.GetHwnd());
		imageRect.right *= 0.5f;

		renderTarget.DrawBitmap(pBitmap, &imageRect);

		ID2D1SolidColorBrush* pRedBrush = nullptr;
		renderTarget.CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Red),
			&pRedBrush
		);

		ID2D1SolidColorBrush* pGreenBrush = nullptr;
		renderTarget.CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Green),
			&pGreenBrush
		);

		ID2D1SolidColorBrush* pBlueBrush = nullptr;
		renderTarget.CreateSolidColorBrush(
			D2D1::ColorF(D2D1::ColorF::Blue),
			&pBlueBrush
		);
		{
			assert(pRedBrush != nullptr);
			assert(pGreenBrush != nullptr);
			assert(pBlueBrush != nullptr);

			uint32_t maxFreq = 0;

			for (int i = 0; i < TABLE_SIZE; ++i)
			{
				const uint32_t r = mFrequencyTable.redTable[i];
				if (r > maxFreq)
				{
					maxFreq = r;
				}

				const uint32_t g = mFrequencyTable.greenTable[i];
				if (g > maxFreq)
				{
					maxFreq = g;
				}

				const uint32_t b = mFrequencyTable.blueTable[i];
				if (b > maxFreq)
				{
					maxFreq = b;
				}
			}

			const float gap = imageRect.right / (TABLE_SIZE + 1);
			const float histHeight = imageRect.bottom * 0.33f;

			const float redBottom = imageRect.bottom;
			const float greenBottom = imageRect.bottom * 0.67f;
			const float blueBottom = imageRect.bottom * 0.33f;

			float x = imageRect.right + gap;
			for (int i = 0; i < TABLE_SIZE; ++i)
			{
				D2D1_POINT_2F redTailPoint = { x, redBottom };

				const float red = remapValue(
					mFrequencyTable.redTable[i],
					0, maxFreq,
					0, histHeight
				);
				D2D1_POINT_2F redHeadPoint = { x, redBottom - red };

				renderTarget.DrawLine(
					redTailPoint,
					redHeadPoint,
					pRedBrush
				);

				D2D1_POINT_2F greenTailPoint = { x, greenBottom };

				const float green = remapValue(
					mFrequencyTable.greenTable[i],
					0, maxFreq,
					0, histHeight
				);
				D2D1_POINT_2F greenHeadPoint = { x, greenBottom - green };

				renderTarget.DrawLine(
					greenTailPoint,
					greenHeadPoint,
					pGreenBrush
				);

				D2D1_POINT_2F blueTailPoint = { x, blueBottom };

				const float blue = remapValue(
					mFrequencyTable.blueTable[i],
					0, maxFreq,
					0, histHeight
				);
				D2D1_POINT_2F blueHeadPoint = { x, blueBottom - blue };

				renderTarget.DrawLine(
					blueTailPoint,
					blueHeadPoint,
					pBlueBrush
				);

				x += gap;
			}
		}
		pBlueBrush->Release();
		pGreenBrush->Release();
		pRedBrush->Release();
	}
	pBitmap->Release();
}
