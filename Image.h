#pragma once

#include <vector>

#include "Debug.h"
#include "TypeDef.h"
#include "MathHelper.h"

class Image final
{
public:
	std::vector<Pixel> Pixels;

	int Width;
	int Height;
	int ChannelCount;

	FrequencyTable FrequencyTable;

public:
	Image(const char* path);
	~Image() = default;
	Image(const Image& other) = default;
	Image& operator=(const Image& other) = default;
};