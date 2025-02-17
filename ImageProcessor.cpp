#include "ImageProcessor.h"

constexpr float DEFAULT_BRIGHTNESS_RATIO_F = 1.f;
constexpr float DEFAULLT_GAMMA_SCALER_F = 1.f;

ImageProcessor::ImageProcessor()
    : mOriginalImage()
    , mBufferedImage()
    , mNormalizedPixels(nullptr)
    , mResultImage()
    , mBrightnessRatio(DEFAULT_BRIGHTNESS_RATIO_F)
    , mGammaScaler(DEFAULT_BRIGHTNESS_RATIO_F)
    , mFlags({ 0, })
    , mDirtyFlags({ 0, })
{
    ImPlot::CreateContext();
}

ImageProcessor::~ImageProcessor()
{
    ImPlot::DestroyContext();

    delete[] mNormalizedPixels;
}

Image& ImageProcessor::GetProcessedImage()
{
    return mResultImage;
}

void ImageProcessor::Update()
{
    if (!mDirtyFlags.flags)
    {
        return;
    }

    if (mDirtyFlags.bits.restoring)
    {
        restoreDefaultAdjustment();

        return;
    }

    if (mDirtyFlags.partition.mode | mDirtyFlags.partition.histogramProcessing)
    {
        mBufferedImage = mOriginalImage;
        if (mFlags.bits.grayScale)
        {
            convertToGrayScale(mBufferedImage);
        }

        switch (mFlags.flags & MASK_HISTOGRAM_PROCESSING)
        {
        case EUIConstant::HISTOGRAM_PROCESSING_EQUALIZATION:
            executeEqualization();
            break;

        case EUIConstant::HISTOGRAM_PROCESSING_MATCHING:
            executeHistogramMatching();
            break;

        default:
            // no action
            break;
        }
    }

    ProcessingFunc processingFuncs[] = {
        &ImageProcessor::normalize,
        &ImageProcessor::modifyBrightness,
        &ImageProcessor::storeResult
    };

    for (int i = 0; i < sizeof(processingFuncs) / sizeof(ProcessingFunc); ++i)
    {
        (this->*processingFuncs[i])();
    }

    mDirtyFlags.flags = EUIConstant::NONE;
}

void ImageProcessor::RegisterImage(Image&& other)
{
    mOriginalImage = std::move(other);
    mBufferedImage = mOriginalImage;

    delete[] mNormalizedPixels;

    mNormalizedPixels = new PixelF[mOriginalImage.Width * mOriginalImage.Height];

    const UIFlags tmpFlags = mFlags;
    mFlags.flags = EUIConstant::NONE;
    mFlags.partition.hardwareAcceleration = tmpFlags.partition.hardwareAcceleration;

    restoreDefaultAdjustment();
}

void ImageProcessor::DrawControlPanel()
{
    ImGui::Begin("Control Panel");
    {
        if (ImPlot::BeginPlot("Histogram"))
        {
            const Histogram hist = mResultImage.GetHistogram();

            ImPlot::SetupAxes("Brightness", "Frequency", 0, ImPlotAxisFlags_AutoFit);

            ImPlot::SetNextFillStyle(ImColor(UINT8_MAX, 0, 0, UINT8_MAX));
            ImPlot::PlotBars("Red", hist.rgbTable.redFrequencyTable, TABLE_SIZE);

            ImPlot::SetNextFillStyle(ImColor(0, UINT8_MAX, 0, UINT8_MAX));
            ImPlot::PlotBars("Green", hist.rgbTable.greenFrequencyTable, TABLE_SIZE);

            ImPlot::SetNextFillStyle(ImColor(0, 0, UINT8_MAX, UINT8_MAX));
            ImPlot::PlotBars("Blue", hist.rgbTable.blueFrequencyTable, TABLE_SIZE);

            ImPlot::EndPlot();
        }

        ImGui::SeparatorText("Hardware Acceleration");
        ImGui::BeginGroup();
        {
            ImGui::CheckboxFlags("SIMD", &mFlags.flags, EUIConstant::HW_SIMD);
            ImGui::SameLine();
            ImGui::CheckboxFlags("CUDA", &mFlags.flags, EUIConstant::HW_CUDA);
        }
        ImGui::EndGroup();

        ImGui::SeparatorText("Mode");
        ImGui::BeginGroup();
        {
            mDirtyFlags.partition.mode = ImGui::CheckboxFlags("GrayScale", &mFlags.flags, EUIConstant::MODE_GRAY_SCALE);
        }
        ImGui::EndGroup();

        ImGui::SeparatorText("Histogram Processing");
        ImGui::BeginGroup();
        {
            UIFlags nextFlags = mFlags;
            nextFlags.bits.equalization = false;
            nextFlags.bits.matching = false;

            mDirtyFlags.partition.histogramProcessing += ImGui::RadioButton("None", reinterpret_cast<int*>(&mFlags), nextFlags.flags);
            ImGui::SameLine();

            nextFlags.bits.equalization = true;
            mDirtyFlags.partition.histogramProcessing += ImGui::RadioButton("Equalization", reinterpret_cast<int*>(&mFlags), nextFlags.flags);

            nextFlags.bits.equalization = false;
            nextFlags.bits.matching = true;
            mDirtyFlags.partition.histogramProcessing += ImGui::RadioButton("Macthing(Choose image to match if dialogbox open)", reinterpret_cast<int*>(&mFlags), nextFlags.flags);
        }
        ImGui::EndGroup();

        ImGui::SeparatorText("Adjustment");
        ImGui::BeginGroup();
        {
            mDirtyFlags.partition.restoring = ImGui::Button("Restore Defaults");

            ImGui::Text("Brightness");
            mDirtyFlags.partition.adjustment += ImGui::SliderFloat("[0, 2] * 100%", &mBrightnessRatio, 0, 2.f, "%.2f");

            ImGui::Text("Gamma");
            mDirtyFlags.partition.adjustment += ImGui::SliderFloat("[0.04, 25]", &mGammaScaler, 0.04f, 25.f, "%.3f");
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}

void ImageProcessor::restoreDefaultAdjustment()
{
    mResultImage = mBufferedImage;

    mDirtyFlags.flags = EUIConstant::NONE;

    mBrightnessRatio = DEFAULT_BRIGHTNESS_RATIO_F;
    mGammaScaler = DEFAULT_BRIGHTNESS_RATIO_F;
}

void ImageProcessor::executeEqualization()
{
    Histogram hist = mBufferedImage.GetHistogram();

    const int pixelCount = mBufferedImage.Width * mBufferedImage.Height;

    if (mFlags.bits.cuda)
    {

    }
    else
    {
        equalizeHistogram(hist, pixelCount);

        for (int i = 0; i < pixelCount; ++i)
        {
            Pixel& pixel = mBufferedImage.pRawPixels[i];
            for (int color = 0; color < COLOR_COUNT; ++color)
            {
                pixel.subPixels[color] = hist.frequencyTables[color][pixel.subPixels[color]];
            }
        }
    }
}

void ImageProcessor::equalizeHistogram(Histogram& outHistogram, const int pixelCount)
{
    uint32_t cumulativeSum[COLOR_COUNT] = { 0, };
    for (int i = 0; i <= MAX_BRIGHTNESS; ++i)
    {
        for (int color = 0; color < COLOR_COUNT; ++color)
        {
            cumulativeSum[color] += outHistogram.frequencyTables[color][i];

            const float newIntensity = roundf(MAX_BRIGHTNESS_F * cumulativeSum[color] / pixelCount);

            outHistogram.frequencyTables[color][i] = static_cast<uint8_t>(newIntensity);
        }
    }
}

void ImageProcessor::executeHistogramMatching()
{
    if (mDirtyFlags.partition.histogramProcessing)
    {
        FileDialog& fileDialog = *FileDialog::GetInstance();
        if (!fileDialog.TryOpenFileDialog(mRefImagePath, EFileDialogConstant::DEFAULT_PATH_LEN))
        {
            mDirtyFlags.partition.histogramProcessing = true;
            mFlags.partition.histogramProcessing = false;

            return;
        }
    }

    Image refImage(mRefImagePath);
    if (mFlags.bits.grayScale)
    {
        convertToGrayScale(refImage);
    }

    Histogram refEqualizedHist = refImage.GetHistogram();

    equalizeHistogram(refEqualizedHist, refImage.Width * refImage.Height);

    const int pixelCount = mBufferedImage.Width * mBufferedImage.Height;

    Histogram equalizedHist = mBufferedImage.GetHistogram();
    equalizeHistogram(equalizedHist, pixelCount);

    Histogram inverseLookup = { 0, };
    for (int i = 0; i <= MAX_BRIGHTNESS; ++i)
    {
        for (int color = 0; color < COLOR_COUNT; ++color)
        {
            int k = MAX_BRIGHTNESS;
            while (refEqualizedHist.frequencyTables[color][k] > equalizedHist.frequencyTables[color][i])
            {
                --k;
            }

            if (k < 0)
            {
                inverseLookup.frequencyTables[color][i] = 0;
            }
            else
            {
                inverseLookup.frequencyTables[color][i] = k;
            }
        }
    }

    for (int i = 0; i < pixelCount; ++i)
    {
        Pixel& pixel = mBufferedImage.pRawPixels[i];
        for (int color = 0; color < COLOR_COUNT; ++color)
        {
            pixel.subPixels[color] = inverseLookup.frequencyTables[color][pixel.subPixels[color]];
        }
    }
}

void ImageProcessor::normalize()
{
    for (int i = 0; i < mBufferedImage.Width * mBufferedImage.Height; ++i)
    {
        const Pixel& pixel = mBufferedImage.pRawPixels[i];

        PixelF& normalizedPixel = mNormalizedPixels[i];
        for (int color = 0; color < COLOR_COUNT; ++color)
        {
            normalizedPixel.subPixels[color] = pixel.subPixels[color] * NORMALIZER_F;
        }
    }
}

void ImageProcessor::modifyBrightness()
{
    if (mFlags.bits.cuda)
    {

    }
    else if (mFlags.bits.simd)
    {

    }
    else
    {
        for (int i = 0; i < mBufferedImage.Width * mBufferedImage.Height; ++i)
        {
            PixelF& pixel = mNormalizedPixels[i];

            for (int color = 0; color < COLOR_COUNT; ++color)
            {
                const float newIntensity = clampNormalizedBrightness(pixel.subPixels[color] * mBrightnessRatio);

                pixel.subPixels[color] = powf(newIntensity, mGammaScaler);
            }
        }
    }
}

void ImageProcessor::storeResult()
{
    for (int i = 0; i < mBufferedImage.Width * mBufferedImage.Height; ++i)
    {
        const PixelF& pixel = mNormalizedPixels[i];

        Pixel& resultPixel = mResultImage.pRawPixels[i];
        for (int color = 0; color < COLOR_COUNT; ++color)
        {
            resultPixel.subPixels[color] = static_cast<uint8_t>(pixel.subPixels[color] * UNNORMALIZER_F);
        }
    }
}

void ImageProcessor::convertToGrayScale(Image& outImage)
{
    if (outImage.ChannelCount <= 2)
    {
        return;
    }

    for (int i = 0; i < outImage.Width * outImage.Height; ++i)
    {
        Pixel& pixel = outImage.pRawPixels[i];

        // luma coding
        const float newIntensity = clampBrightness(pixel.rgba.r * 0.299f + pixel.rgba.g * 0.587f + pixel.rgba.b * 0.114f);
        const uint8_t grayBrightness = static_cast<uint8_t>(newIntensity);

        for (int color = 0; color < COLOR_COUNT; ++color)
        {
            pixel.subPixels[color] = grayBrightness;
        }
    }
}
