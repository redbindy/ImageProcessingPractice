#pragma once

#include <Windows.h>
#include <vector>

#include "Debug.h"
#include "TypeDef.h"
#include "Image.h"

void EqualizeHelperCPU(Image& outImage);

struct EqualizeArgs
{
	Pixel* pPixels;
	const int pixelCount;
	FrequencyTable* pFrequencyTable;
	const EHandleColor color;
};

DWORD WINAPI EqualizeThread(void* pArgs);