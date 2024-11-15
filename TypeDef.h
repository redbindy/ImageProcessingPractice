#pragma once
#include <cstdint>
#include "Debug.h"
#include "vector_types.h"

enum
{
	TABLE_SIZE = 256
};

struct FrequencyTable
{
	uint32_t redTable[TABLE_SIZE];
	uint32_t greenTable[TABLE_SIZE];
	uint32_t blueTable[TABLE_SIZE];
};

union Pixel
{
	struct rgba
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} rgba;

	uint32_t pixel;
};
// static_assert(sizeof(Pixel) == 4);

enum EHandleColor
{
	RED,
	GREEN,
	BLUE,
	COUNT
};