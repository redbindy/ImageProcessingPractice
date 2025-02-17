#pragma once

#include <cstdint>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <implot.h>

#include <cmath>

#include "Debug.h"
#include "Image.h"
#include "ComHelper.h"

#include "FileDialog.h"

class ImageProcessor final
{
public:
    ImageProcessor();
    ~ImageProcessor();
    ImageProcessor(const ImageProcessor& other) = delete;
    ImageProcessor(ImageProcessor&& other) = delete;
    ImageProcessor& operator=(const ImageProcessor& other) = delete;
    ImageProcessor& operator=(ImageProcessor&& other) = delete;

    Image& GetProcessedImage();
    void Update();

    void RegisterImage(Image&& other);
    void DrawControlPanel();

private:
    enum EUIConstant
    {
        NONE = 0,

        // Hardware Acceleration
        HW_SIMD = 1,
        HW_CUDA = 1 << 1,

        // Mode
        MODE_GRAY_SCALE = 1 << 2,

        // Histogram Processing
        HISTOGRAM_PROCESSING_EQUALIZATION = 1 << 3,
        HISTOGRAM_PROCESSING_MATCHING = 1 << 4,

        // Mask
        MASK_HISTOGRAM_PROCESSING = HISTOGRAM_PROCESSING_EQUALIZATION | HISTOGRAM_PROCESSING_MATCHING
    };

    union UIFlags
    {
        struct
        {
            // Hardware Acceleration
            uint32_t simd : 1;
            uint32_t cuda : 1;

            // Mode
            uint32_t grayScale : 1;

            // Histogram Processing
            uint32_t equalization : 1;
            uint32_t matching : 1;

            // Adjustment
            uint32_t restoring : 1;
            uint32_t reserved : 26;
        } bits;

        struct
        {
            uint32_t hardwareAcceleration : 2;
            uint32_t mode : 1;
            uint32_t histogramProcessing : 2;
            uint32_t restoring : 1;
            uint32_t adjustment : 26;
        } partition;

        uint32_t flags;
    };
    static_assert(sizeof(UIFlags) == 4, "UIFlags should be 4 bytes");

    using ProcessingFunc = void (ImageProcessor::*)();

private:
    Image mOriginalImage;

    Image mBufferedImage;
    PixelF* mNormalizedPixels;

    Image mResultImage;

    UIFlags mFlags;
    UIFlags mDirtyFlags;

    char mRefImagePath[EFileDialogConstant::DEFAULT_PATH_LEN];

    float mBrightnessRatio;

private:
    void restoreDefaultAdjustment();

    template<typename T>
    T clamp(T value, T min, T max) const;

    template<typename T>
    T clampBrightness(T value) const;

    template<typename T>
    T clampNormalizedBrightness(T value) const;

    void convertToGrayScale(Image& outImage);

    void executeEqualization();
    void equalizeHistogram(Histogram& outHistogram, const int pixelCount);
    void executeHistogramMatching();

    void normalize();
    void modifyBrightness();
    void storeResult();
};

template<typename T>
inline T ImageProcessor::clamp(T value, T min, T max) const
{
    if (value < min)
    {
        return min;
    }

    if (value > max)
    {
        return max;
    }

    return value;
}

template<typename T>
inline T ImageProcessor::clampBrightness(T value) const
{
    return clamp(value, MIN_BRIGHTNESS_F, MAX_BRIGHTNESS_F);
}

template<typename T>
inline T ImageProcessor::clampNormalizedBrightness(T value) const
{
    return clamp(value, NORMALIZED_MIN_F, NORMALIZED_MAX_F);
}
