#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image::Image(const char* path)
	: FrequencyTable{ 0, }
{
	ASSERT(path != nullptr);

	unsigned char* pPixelData = stbi_load(
		path,
		&Width,
		&Height,
		&ChannelCount,
		0
	);
	assert(pPixelData != nullptr);
	ASSERT(ChannelCount >= 3, "Image channel should be 3 >=");
	{
		Pixels.reserve(Width * Height);

		for (int y = 0; y < Height; ++y)
		{
			for (int x = 0; x < Width; ++x)
			{
				const int index = ToIndex(x, y, Width) * ChannelCount;

				Pixel pixel;

				pixel.rgba.r = pPixelData[index];
				pixel.rgba.g = pPixelData[index + 1];
				pixel.rgba.b = pPixelData[index + 2];
				pixel.rgba.a = UINT8_MAX;

				++FrequencyTable.redTable[pixel.rgba.r];
				++FrequencyTable.greenTable[pixel.rgba.g];
				++FrequencyTable.blueTable[pixel.rgba.b];

				Pixels.push_back(pixel);
			}
		}
	}
	stbi_image_free(pPixelData);
}
