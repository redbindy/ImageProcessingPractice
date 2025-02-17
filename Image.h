#pragma once

#include <cstdint>
#include <cassert>
#include <cstring>

#include <Windows.h>

enum EImageConstant
{
    MAX_CHANNEL_COUNT = 4,
    TABLE_SIZE = UINT8_MAX + 1,

    COLOR_COUNT = 3
};

// min, max brightness
constexpr int MIN_BRIGHTNESS = 0;
constexpr int MAX_BRIGHTNESS = 255;
constexpr float MIN_BRIGHTNESS_F = 0.f;
constexpr float MAX_BRIGHTNESS_F = 255.f;
constexpr float NORMALIZED_MIN_F = 0.f;
constexpr float NORMALIZED_MAX_F = 1.f;

// min max scaling
constexpr float NORMALIZER_F = 1 / 255.f;
constexpr float UNNORMALIZER_F = 255.f;

union Pixel
{
    // dxgi bgra
    struct
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    } rgba;

    uint8_t subPixels[MAX_CHANNEL_COUNT];

    uint32_t pixel;
};

union PixelF
{
    struct
    {
        float b;
        float g;
        float r;
        float a;
    } rgba;

    float subPixels[MAX_CHANNEL_COUNT];
};

union Histogram
{
    struct
    {
        uint32_t blueFrequencyTable[EImageConstant::TABLE_SIZE];
        uint32_t greenFrequencyTable[EImageConstant::TABLE_SIZE];
        uint32_t redFrequencyTable[EImageConstant::TABLE_SIZE];
    } rgbTable;

    uint32_t frequencyTables[COLOR_COUNT][EImageConstant::TABLE_SIZE];
};

class App;
class ImageProcessor;

class Image final
{
    friend App;
    friend ImageProcessor;

public:
    Image(const char* path);
    ~Image();
    Image(const Image& other);
    Image(Image&& other);
    Image& operator=(const Image& other);
    Image& operator=(Image&& other);

    Histogram GetHistogram() const;

public:
    Pixel* pRawPixels;

    int Width;
    int Height;
    int ChannelCount;

private:
    Image();

    inline int convertToIndex(const int x, const int y) const;
};

inline int Image::convertToIndex(const int x, const int y) const
{
    assert(x >= 0);
    assert(y >= 0);

    return y * Width + x;
}
