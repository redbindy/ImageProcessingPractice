#pragma once

#include "TypeDef.h"

struct ImageDTOForGPU
{
	Pixel* pPixels;
	const int width;
	const int height;
	FrequencyTable* pFrequencyTable;
};

void EqualizeHelperGPU(ImageDTOForGPU image);

void Match();