#include "ProcessingHelperCPU.h"

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

DWORD __stdcall EqualizeThread(void* pArgs)
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
