#pragma once

#include "TypeDef.h"

void EqualizeHelperGPU(
	Pixel* pPixels, 
	const int imageWidth, 
	const int imageHeight, 
	FrequencyTable* pFrequencyTable
);
