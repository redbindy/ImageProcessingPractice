#pragma once

#include <Windows.h>
#include <vector>

#include "Debug.h"
#include "TypeDef.h"

void EqualizeHelperCPU(std::vector<Pixel>& pixels, FrequencyTable& frequencyTable);

struct EqualizeArgs
{
	Pixel* pPixels;
	const int pixelCount;
	FrequencyTable* pFrequencyTable;
	const EHandleColor color;
};

DWORD WINAPI EqualizeThread(void* pArgs);