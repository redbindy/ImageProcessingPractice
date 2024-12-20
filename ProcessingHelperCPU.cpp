#include "ProcessingHelperCPU.h"

struct EqualizeArgs
{
	Pixel* pPixels;
	const int pixelCount;
	FrequencyTable* pFrequencyTable;
	const EHandleColor color;
};

static DWORD __stdcall EqualizeThread(void* pArgs)
{
	assert(pArgs != nullptr);

	EqualizeArgs* pEqualizeArgs = (EqualizeArgs*)(pArgs);

	Pixel* pPixels = pEqualizeArgs->pPixels;
	FrequencyTable* pFreqeuncyTable = pEqualizeArgs->pFrequencyTable;

	const int pixelCount = pEqualizeArgs->pixelCount;
	const EHandleColor handlingColor = pEqualizeArgs->color;

	float cdf[TABLE_SIZE];

	uint32_t freqSum = 0;
	for (int i = 0; i < TABLE_SIZE; ++i)
	{
		switch (handlingColor)
		{
		case EHandleColor::RED:
			freqSum += pFreqeuncyTable->redTable[i];
			break;

		case EHandleColor::GREEN:
			freqSum += pFreqeuncyTable->greenTable[i];
			break;

		case EHandleColor::BLUE:
			freqSum += pFreqeuncyTable->blueTable[i];
			break;

		default:
			ASSERT(false);
			break;
		}

		cdf[i] = freqSum / (float)pixelCount;
	}

	switch (handlingColor)
	{
	case EHandleColor::RED:
		memset(pFreqeuncyTable->redTable, 0, sizeof(pFreqeuncyTable->redTable));
		break;

	case EHandleColor::GREEN:
		memset(pFreqeuncyTable->greenTable, 0, sizeof(pFreqeuncyTable->greenTable));
		break;

	case EHandleColor::BLUE:
		memset(pFreqeuncyTable->blueTable, 0, sizeof(pFreqeuncyTable->blueTable));
		break;

	default:
		ASSERT(false);
		break;
	}

	for (Pixel* pPixel = pPixels; pPixel < pPixels + pixelCount; ++pPixel)
	{
		switch (handlingColor)
		{
		case EHandleColor::RED:
			pPixel->rgba.r = (uint8_t)round(UINT8_MAX * cdf[pPixel->rgba.r]);
			++pFreqeuncyTable->redTable[pPixel->rgba.r];
			break;

		case EHandleColor::GREEN:
			pPixel->rgba.g = (uint8_t)round(UINT8_MAX * cdf[pPixel->rgba.g]);
			++pFreqeuncyTable->greenTable[pPixel->rgba.g];
			break;

		case EHandleColor::BLUE:
			pPixel->rgba.b = (uint8_t)round(UINT8_MAX * cdf[pPixel->rgba.b]);
			++pFreqeuncyTable->blueTable[pPixel->rgba.b];
			break;

		default:
			ASSERT(false);
			break;
		}
	}

	return 0;
}

void EqualizeHelperCPU(Image& outImage)
{
	EqualizeArgs argsArr[EHandleColor::COUNT] = {
		{ outImage.Pixels.data(), static_cast<const int>(outImage.Pixels.size()), &outImage.FrequencyTable, EHandleColor::RED},
		{ outImage.Pixels.data(), static_cast<const int>(outImage.Pixels.size()), &outImage.FrequencyTable, EHandleColor::GREEN },
		{ outImage.Pixels.data(), static_cast<const int>(outImage.Pixels.size()), &outImage.FrequencyTable, EHandleColor::BLUE }
	};

	HANDLE handles[EHandleColor::COUNT];
	for (int i = 0; i < EHandleColor::COUNT; ++i)
	{
		handles[i] = CreateThread(nullptr, 0, EqualizeThread, argsArr + i, 0, nullptr);
		ASSERT(handles[i] != nullptr);
	}

	DWORD result = WaitForMultipleObjects(EHandleColor::COUNT, handles, true, INFINITE);
	ASSERT(result == WAIT_OBJECT_0);
}

static void CorrectGamma(Image& outImage, const int begin, const int end, const float gamma)
{
	constexpr float NORMALIZER = 1 / 255.f;
	constexpr float UNNORMALIZER = 255.f;

	const __m256 normalizingScaler = _mm256_broadcast_ss(&NORMALIZER);
	const __m256 gammaVector = _mm256_broadcast_ss(&gamma);
	const __m256 unnormalizingScaler = _mm256_broadcast_ss(&UNNORMALIZER);

	const __m256i zeroVector = _mm256_setzero_si256();
	for (int i = begin; i < end; i += sizeof(__m256) / sizeof(Pixel))
	{
		__m256i pixelVector = _mm256_loadu_epi32(outImage.Pixels.data() + i);

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

		_mm256_storeu_epi8(outImage.Pixels.data() + i, scaledVector);
	}
}

void GammaHelperCPU(Image& outImage, const float gamma)
{
	constexpr int NUM_THREAD = 6;
	const int blockSize = static_cast<int>(outImage.Pixels.size() / NUM_THREAD);

	std::thread threadList[NUM_THREAD];

	for (int i = 0; i < NUM_THREAD; ++i)
	{
		const int begin = i * blockSize;
		const int end = (i + 1) * blockSize;

		threadList[i] = std::thread(CorrectGamma, std::ref(outImage), begin, end, gamma);
	}

	for (int i = 0; i < NUM_THREAD; ++i)
	{
		threadList[i].join();
	}
}
