#include "ImageProcessor.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

ImageProcessor::ImageProcessor(const char* path)
	: mImageWidth(0)
	, mImageHeight(0)
	, mImageChannelCount(0)
	, mDrawMode(EDrawMode::DEFAULT)
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
				++mFrequencyTable.greenTable[pixel.rgba.g];
				++mFrequencyTable.blueTable[pixel.rgba.b];

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

void ImageProcessor::SetDrawMode(const EDrawMode mode)
{
	mDrawMode = mode;
}

ID2D1Bitmap* ImageProcessor::createRenderedBitmapHeap(ID2D1HwndRenderTarget& renderTarget) const
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

void ImageProcessor::DrawImage(ID2D1HwndRenderTarget& renderTarget)
{
	switch (mDrawMode)
	{
	case EDrawMode::DEFAULT:
		drawImageDefault(renderTarget);
		break;

	case EDrawMode::HISTOGRAM:
		drawImageHistogram(renderTarget);
		break;

	case EDrawMode::EQUALIZATION:
		drawImageEqualization(renderTarget);
		break;

	default:
		ASSERT(false);
		break;
	}
}

void ImageProcessor::drawImageDefault(ID2D1HwndRenderTarget& renderTarget) const
{
	ID2D1Bitmap* pBitmap = createRenderedBitmapHeap(renderTarget);
	{
		const D2D1_RECT_F imageRect = getD2DRect(renderTarget.GetHwnd());

		renderTarget.DrawBitmap(pBitmap, imageRect);
	}
	pBitmap->Release();
}


void ImageProcessor::drawImageHistogram(ID2D1HwndRenderTarget& renderTarget) const
{
	ID2D1Bitmap* pBitmap = createRenderedBitmapHeap(renderTarget);
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
					static_cast<float>(mFrequencyTable.redTable[i]),
					0, static_cast<float>(maxFreq),
					0, static_cast<float>(histHeight)
				);
				D2D1_POINT_2F redHeadPoint = { x, redBottom - red };

				renderTarget.DrawLine(
					redTailPoint,
					redHeadPoint,
					pRedBrush
				);

				D2D1_POINT_2F greenTailPoint = { x, greenBottom };

				const float green = remapValue(
					static_cast<float>(mFrequencyTable.greenTable[i]),
					0, static_cast<float>(maxFreq),
					0, static_cast<float>(histHeight)
				);
				D2D1_POINT_2F greenHeadPoint = { x, greenBottom - green };

				renderTarget.DrawLine(
					greenTailPoint,
					greenHeadPoint,
					pGreenBrush
				);

				D2D1_POINT_2F blueTailPoint = { x, blueBottom };

				const float blue = remapValue(
					static_cast<float>(mFrequencyTable.blueTable[i]),
					0, static_cast<float>(maxFreq),
					0, static_cast<float>(histHeight)
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

void ImageProcessor::drawImageEqualization(ID2D1HwndRenderTarget& renderTarget)
{
	// TODO:
	std::vector<Pixel> backupPixels = mPixels;
	FrequencyTable backupFreqTable = mFrequencyTable;
	{
		clock_t start = clock();
		{
			float redCDF[TABLE_SIZE];
			float greenCDF[TABLE_SIZE];
			float blueCDF[TABLE_SIZE];

			uint32_t redSum = 0;
			uint32_t greenSum = 0;
			uint32_t blueSum = 0;
			for (int i = 0; i < TABLE_SIZE; ++i)
			{
				redSum += mFrequencyTable.redTable[i];
				redCDF[i] = redSum / static_cast<float>(mImageWidth * mImageHeight);

				greenSum += mFrequencyTable.greenTable[i];
				greenCDF[i] = greenSum / static_cast<float>(mImageWidth * mImageHeight);

				blueSum += mFrequencyTable.blueTable[i];
				blueCDF[i] = blueSum / static_cast<float>(mImageWidth * mImageHeight);
			}

			for (Pixel& pixel : mPixels)
			{
				pixel.rgba.r = static_cast<uint8_t>(round(UINT8_MAX * redCDF[pixel.rgba.r]));
				pixel.rgba.g = static_cast<uint8_t>(round(UINT8_MAX * greenCDF[pixel.rgba.g]));
				pixel.rgba.b = static_cast<uint8_t>(round(UINT8_MAX * blueCDF[pixel.rgba.b]));
			}

			memset(&mFrequencyTable, 0, sizeof(mFrequencyTable));

			for (const Pixel& pixel : mPixels)
			{
				++mFrequencyTable.redTable[pixel.rgba.r];
				++mFrequencyTable.greenTable[pixel.rgba.g];
				++mFrequencyTable.blueTable[pixel.rgba.b];
			}
		}
		clock_t end = clock();
		std::cout << (end - start) / double(CLOCKS_PER_SEC) << std::endl;

		drawImageHistogram(renderTarget);
	}
	mFrequencyTable = backupFreqTable;
	backupPixels.swap(mPixels);
}
