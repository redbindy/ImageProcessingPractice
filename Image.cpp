#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Image::Image()
    : pRawPixels(nullptr)
    , Width(0)
    , Height(0)
    , ChannelCount(0)
{
}

Image::Image(const char* path)
    : Image()
{
    assert(path != nullptr);

    unsigned char* pData = stbi_load(path, &Width, &Height, &ChannelCount, 0);
    assert(pData != nullptr);
    assert(Width > 0);
    assert(Height > 0);
    assert(ChannelCount > 0 && ChannelCount <= MAX_CHANNEL_COUNT);
    {
        pRawPixels = new Pixel[Width * Height];

        for (int i = 0; i < Width * Height; ++i)
        {
            const int subPixelIndex = i * ChannelCount;

            Pixel pixel;
            switch (ChannelCount)
            {
            case 1:
                pixel.rgba.r = pData[subPixelIndex];
                pixel.rgba.g = pixel.rgba.r;
                pixel.rgba.b = pixel.rgba.r;
                pixel.rgba.a = UINT8_MAX;
                break;

            case 2:
                pixel.rgba.r = pData[subPixelIndex];
                pixel.rgba.g = pixel.rgba.r;
                pixel.rgba.b = pixel.rgba.r;
                pixel.rgba.a = pData[subPixelIndex + 1];
                break;

            case 3:
                pixel.rgba.r = pData[subPixelIndex];
                pixel.rgba.g = pData[subPixelIndex + 1];
                pixel.rgba.b = pData[subPixelIndex + 2];
                pixel.rgba.a = UINT8_MAX;
                break;

            case 4:
                pixel.rgba.r = pData[subPixelIndex];
                pixel.rgba.g = pData[subPixelIndex + 1];
                pixel.rgba.b = pData[subPixelIndex + 2];
                pixel.rgba.a = pData[subPixelIndex + 3];
                break;

            default:
                assert(false);
                break;
            }

            pRawPixels[i] = pixel;
        }
    }
    stbi_image_free(pData);
}

Image::~Image()
{
    delete[] pRawPixels;
}

Image::Image(const Image& other)
    : pRawPixels(nullptr)
    , Width(other.Width)
    , Height(other.Height)
    , ChannelCount(other.ChannelCount)
{
    assert(other.pRawPixels != nullptr);
    assert(Width > 0);
    assert(Height > 0);
    assert(ChannelCount > 0 && ChannelCount <= MAX_CHANNEL_COUNT);

    pRawPixels = new Pixel[Width * Height];

    memcpy(pRawPixels, other.pRawPixels, sizeof(Pixel) * Width * Height);
}

Image::Image(Image&& other)
    : pRawPixels(other.pRawPixels)
    , Width(other.Width)
    , Height(other.Height)
    , ChannelCount(other.ChannelCount)
{
    assert(other.pRawPixels != nullptr);
    assert(Width > 0);
    assert(Height > 0);
    assert(ChannelCount > 0 && ChannelCount <= MAX_CHANNEL_COUNT);

    other.pRawPixels = nullptr;
}

Image& Image::operator=(const Image& other)
{
    assert(other.pRawPixels != nullptr);
    assert(other.Width > 0);
    assert(other.Height > 0);
    assert(other.ChannelCount > 0 && other.ChannelCount <= MAX_CHANNEL_COUNT);

    if (this != &other)
    {
        if (Width != other.Width || Height != other.Height)
        {
            delete[] pRawPixels;

            Width = other.Width;
            Height = other.Height;
            ChannelCount = other.ChannelCount;

            pRawPixels = new Pixel[Width * Height];
        }

        memcpy(pRawPixels, other.pRawPixels, sizeof(Pixel) * Width * Height);
    }

    return *this;
}

Image& Image::operator=(Image&& other)
{
    assert(other.pRawPixels != nullptr);
    assert(other.Width > 0);
    assert(other.Height > 0);
    assert(other.ChannelCount > 0 && other.ChannelCount <= MAX_CHANNEL_COUNT);

    if (this != &other)
    {
        delete[] pRawPixels;

        pRawPixels = other.pRawPixels;
        Width = other.Width;
        Height = other.Height;
        ChannelCount = other.ChannelCount;

        other.pRawPixels = nullptr;
    }

    return *this;
}

struct Args
{
    const Pixel* pPixels;
    const int width;
    const int height;

    Histogram* pOutHistogram;
    const int threadId;
};

DWORD WINAPI getHistogramThread(VOID* const pParam)
{
    assert(pParam != nullptr);

    Args* const pArgs = reinterpret_cast<Args*>(pParam);

    const int threadId = pArgs->threadId;
    uint32_t* pOutTable = pArgs->pOutHistogram->frequencyTables[threadId];

    const Pixel* pPixels = pArgs->pPixels;
    const int pixelCount = pArgs->width * pArgs->height;

    for (int i = 0; i < pixelCount; ++i)
    {
        ++pOutTable[pPixels[i].subPixels[threadId]];
    }

    return 0;
}

Histogram Image::GetHistogram() const
{
    assert(pRawPixels != nullptr);

    Histogram hist = { 0, };

    Args argArr[COLOR_COUNT] = {
        { pRawPixels, Width, Height, &hist, 0 },
        { pRawPixels, Width, Height, &hist, 1 },
        { pRawPixels, Width, Height, &hist, 2 }
    };

    HANDLE threadHandles[COLOR_COUNT];

    for (int i = 0; i < COLOR_COUNT; ++i)
    {
        threadHandles[i] = CreateThread(nullptr, 0, getHistogramThread, argArr + i, 0, 0);
    }

    WaitForMultipleObjects(COLOR_COUNT, threadHandles, true, INFINITE);

    return hist;
}
