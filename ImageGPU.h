#pragma once

#include "TypeDef.h"
#include "ProcessingHelperGPU.h"

struct ImageDTOForGPU
{
	Pixel* pPixels;
	const int width;
	const int height;
	FrequencyTable* pFrequencyTable;
};

class ImageGPU
{
public:
	Pixel* pPixel;
	const int Width;
	const int Height;
	FrequencyTable FrequencyTable;

public:
	ImageGPU() = delete;
	ImageGPU(const ImageDTOForGPU& image);
	ImageGPU(const ImageGPU& other) = default;
	ImageGPU& operator=(const ImageGPU& other) = default;
	~ImageGPU();
};