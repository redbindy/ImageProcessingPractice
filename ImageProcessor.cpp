#include "ImageProcessor.h"

ImageProcessor::ImageProcessor(const char* path)
	: mImage(path)
	, mDrawMode(EDrawMode::DEFAULT)
{

}

void ImageProcessor::SetDrawMode(const EDrawMode mode)
{
	mDrawMode = mode;
}

ID2D1Bitmap* ImageProcessor::createRenderedBitmapHeap(ID2D1HwndRenderTarget& renderTarget) const
{
	ID2D1Bitmap* pBitmap = nullptr;

	const D2D1_SIZE_U size = {
		static_cast<UINT32>(mImage.Width),
		static_cast<UINT32>(mImage.Height)
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
		mImage.Pixels.data(),
		sizeof(Pixel) * size.width
	);

	return pBitmap;
}

D2D1_RECT_F ImageProcessor::getD2DRect(const HWND hWnd) const
{
	assert(hWnd != nullptr);

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
	clock_t start = clock();
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

		case EDrawMode::MATCHING:
			drawImageMatching(renderTarget);
			break;

		case EDrawMode::GAMMA:
			drawImageGamma(renderTarget);
			break;

		default:
			ASSERT(false);
			break;
		}
	}
	clock_t end = clock();
	std::cout << (end - start) / double(CLOCKS_PER_SEC) << std::endl;
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
				const uint32_t r = mImage.FrequencyTable.redTable[i];
				if (r > maxFreq)
				{
					maxFreq = r;
				}

				const uint32_t g = mImage.FrequencyTable.greenTable[i];
				if (g > maxFreq)
				{
					maxFreq = g;
				}

				const uint32_t b = mImage.FrequencyTable.blueTable[i];
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

				const float red = RemapValue(
					static_cast<float>(mImage.FrequencyTable.redTable[i]),
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

				const float green = RemapValue(
					static_cast<float>(mImage.FrequencyTable.greenTable[i]),
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

				const float blue = RemapValue(
					static_cast<float>(mImage.FrequencyTable.blueTable[i]),
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
	std::vector<Pixel> backupPixels = mImage.Pixels;
	FrequencyTable backupFreqTable = mImage.FrequencyTable;
	{
#if false /* SERIAL */
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

		memset(&mFrequencyTable, 0, sizeof(mFrequencyTable));

		for (Pixel& pixel : mPixels)
		{
			pixel.rgba.r = static_cast<uint8_t>(round(UINT8_MAX * redCDF[pixel.rgba.r]));
			++mFrequencyTable.redTable[pixel.rgba.r];

			pixel.rgba.g = static_cast<uint8_t>(round(UINT8_MAX * greenCDF[pixel.rgba.g]));
			++mFrequencyTable.greenTable[pixel.rgba.g];

			pixel.rgba.b = static_cast<uint8_t>(round(UINT8_MAX * blueCDF[pixel.rgba.b]));
			++mFrequencyTable.blueTable[pixel.rgba.b];
		}
#endif /* SERIAL */

#if false /* 3 THREAD */
		EqualizeHelperCPU(mPixels, mFrequencyTable);
#endif /* 3 THREAD */

#if true /* CUDA */

		ImageDTOForGPU imageDTO = {
			mImage.Pixels.data(),
			mImage.Width,
			mImage.Height,
			&(mImage.FrequencyTable)
		};

		EqualizeHelperGPU(imageDTO);
#endif /* CUDA */

		drawImageHistogram(renderTarget);
	}
	mImage.FrequencyTable = backupFreqTable;
	backupPixels.swap(mImage.Pixels);
}

void ImageProcessor::drawImageMatching(ID2D1HwndRenderTarget& renderTarget)
{
	Image image("xenoblade2-01.jpg");

	std::vector<Pixel> backupPixels = mImage.Pixels;
	FrequencyTable backupFreqTable = mImage.FrequencyTable;
	{
#if false /* SERIAL */
		ImageDTOForGPU srcImageDTO = {
			mImage.Pixels.data(),
			mImage.Width,
			mImage.Height,
			&(mImage.FrequencyTable)
		};

		EqualizeHelperGPU(srcImageDTO);

		ImageDTOForGPU refImageDTO = {
			image.Pixels.data(),
			image.Width,
			image.Height,
			&(image.FrequencyTable)
		};

		EqualizeHelperGPU(refImageDTO);

		FrequencyTable lookup = { 0, };
		for (int i = 0; i < TABLE_SIZE; ++i)
		{
			int k;
			for (k = 0; k < TABLE_SIZE; ++k)
			{
				if (image.FrequencyTable.redTable[k] > mImage.FrequencyTable.redTable[i])
				{
					break;
				}
			}

			lookup.redTable[i] = k;

			for (k = 0; k < TABLE_SIZE; ++k)
			{
				if (image.FrequencyTable.greenTable[k] > mImage.FrequencyTable.greenTable[i])
				{
					break;
				}
			}

			lookup.greenTable[i] = k;

			for (k = 0; k < TABLE_SIZE; ++k)
			{
				if (image.FrequencyTable.blueTable[k] > mImage.FrequencyTable.blueTable[i])
				{
					break;
				}
			}

			lookup.blueTable[i] = k;
		}

		image.Pixels.clear();
		memset(&(image.FrequencyTable), 0, sizeof(FrequencyTable));
		for (int i = 0; i < mImage.Pixels.size(); ++i)
		{
			Pixel newPixel;
			newPixel.rgba.r = lookup.redTable[mImage.Pixels[i].rgba.r];
			++image.FrequencyTable.redTable[newPixel.rgba.r];

			newPixel.rgba.g = lookup.greenTable[mImage.Pixels[i].rgba.g];
			++image.FrequencyTable.greenTable[newPixel.rgba.g];

			newPixel.rgba.b = lookup.blueTable[mImage.Pixels[i].rgba.b];
			++image.FrequencyTable.blueTable[newPixel.rgba.b];

			newPixel.rgba.a = UINT8_MAX;

			image.Pixels.push_back(newPixel);
		}

		mImage.Pixels.swap(image.Pixels);
		mImage.FrequencyTable = image.FrequencyTable;
#endif /* SERIAL */

#if true /* CUDA */
		ImageDTOForGPU srcImageDTO = {
			mImage.Pixels.data(),
			mImage.Width,
			mImage.Height,
			&(mImage.FrequencyTable)
		};

		ImageDTOForGPU refImageDTO = {
			image.Pixels.data(),
			image.Width,
			image.Height,
			&(image.FrequencyTable)
		};

		MatchHelperGPU(srcImageDTO, srcImageDTO, refImageDTO);
#endif /* CUDA */

		drawImageHistogram(renderTarget);
	}
	mImage.Pixels.swap(backupPixels);
	mImage.FrequencyTable = backupFreqTable;
}

void ImageProcessor::drawImageGamma(ID2D1HwndRenderTarget& renderTarget)
{
	std::vector<Pixel> backupPixels = mImage.Pixels;
	FrequencyTable backupFreqTable = mImage.FrequencyTable;
	{
		constexpr float GAMMA = 2.5;

#if false /* SERIAL */
		constexpr float NORMALIZING_SCALER = 1 / 255.f;
		constexpr float UNNORMALIZING_SCALER = 255.f;

		/*for (Pixel& p : mImage.Pixels)
		{
			p.rgba.r = static_cast<uint8_t>(round(pow(p.rgba.r * NORMALIZING_SCALER, GAMMA) * UNNORMALIZING_SCALER));
			p.rgba.g = static_cast<uint8_t>(round(pow(p.rgba.g * NORMALIZING_SCALER, GAMMA) * UNNORMALIZING_SCALER));
			p.rgba.b = static_cast<uint8_t>(round(pow(p.rgba.b * NORMALIZING_SCALER, GAMMA) * UNNORMALIZING_SCALER));
		}*/

		const __m256 normalizingScaler = _mm256_broadcast_ss(&NORMALIZING_SCALER);
		const __m256 gammaVector = _mm256_broadcast_ss(&GAMMA);
		const __m256 unnormalizingScaler = _mm256_broadcast_ss(&UNNORMALIZING_SCALER);

		const __m256i zeroVector = _mm256_setzero_si256();
		for (int i = 0; i < mImage.Pixels.size(); i += sizeof(__m256) / sizeof(Pixel))
		{
			__m256i pixelVector = _mm256_loadu_epi32(mImage.Pixels.data() + i);

			__m256i vector16l = _mm256_unpacklo_epi8(pixelVector, zeroVector);
			__m256i vector16h = _mm256_unpackhi_epi8(pixelVector, zeroVector);

			__m256i vector32lh = _mm256_unpackhi_epi16(vector16l, zeroVector);
			__m256i vector32ll = _mm256_unpacklo_epi16(vector16l, zeroVector);

			__m256i vector32hl = _mm256_unpacklo_epi16(vector16h, zeroVector);
			__m256i vector32hh = _mm256_unpackhi_epi16(vector16h, zeroVector);

			__m256 fvec0 = _mm256_cvtepi32_ps(vector32ll);
			__m256 fvec1 = _mm256_cvtepi32_ps(vector32lh);
			__m256 fvec2 = _mm256_cvtepi32_ps(vector32hl);
			__m256 fvec3 = _mm256_cvtepi32_ps(vector32hh);

			fvec0 = _mm256_mul_ps(fvec0, normalizingScaler);
			fvec1 = _mm256_mul_ps(fvec1, normalizingScaler);
			fvec2 = _mm256_mul_ps(fvec2, normalizingScaler);
			fvec3 = _mm256_mul_ps(fvec3, normalizingScaler);

			fvec0 = powfAVX(fvec0, gammaVector);
			fvec1 = powfAVX(fvec1, gammaVector);
			fvec2 = powfAVX(fvec2, gammaVector);
			fvec3 = powfAVX(fvec3, gammaVector);

			fvec0 = _mm256_mul_ps(fvec0, unnormalizingScaler);
			fvec1 = _mm256_mul_ps(fvec1, unnormalizingScaler);
			fvec2 = _mm256_mul_ps(fvec2, unnormalizingScaler);
			fvec3 = _mm256_mul_ps(fvec3, unnormalizingScaler);

			vector32ll = _mm256_cvtps_epi32(fvec0);
			vector32lh = _mm256_cvtps_epi32(fvec1);
			vector32hl = _mm256_cvtps_epi32(fvec2);
			vector32hh = _mm256_cvtps_epi32(fvec3);

			vector16l = _mm256_packus_epi32(vector32ll, vector32lh);
			vector16h = _mm256_packus_epi32(vector32hl, vector32hh);

			__m256i scaledVector = _mm256_packus_epi16(vector16l, vector16h);

			_mm256_storeu_epi8(mImage.Pixels.data() + i, scaledVector);
		}
#endif /* SERIAL */

#if false /* 6 THREAD */
		GammaHelperCPU(mImage, GAMMA);
#endif /* 6 THREAD */

#if true /* CUDA */
		ImageDTOForGPU imageDTO = {
			mImage.Pixels.data(),
			mImage.Width,
			mImage.Height,
			&(mImage.FrequencyTable)
		};

		GammaHelperGPU(imageDTO, GAMMA);
#endif /* CUDA */

		drawImageDefault(renderTarget);
	}
	mImage.Pixels.swap(backupPixels);
	mImage.FrequencyTable = backupFreqTable;
}
