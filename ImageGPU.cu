#include "ImageGPU.h"

#include "cuda_runtime_api.h"
#include "device_launch_parameters.h"

ImageGPU::ImageGPU(const ImageDTOForGPU& image)
	: pPixel(nullptr)
	, Width(image.width)
	, Height(image.height)
	, FrequencyTable{ 0, }
{
	cudaError_t errorCode = cudaMalloc(&pPixel, sizeof(Pixel) * Width * Height);
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
	}
}

ImageGPU::~ImageGPU()
{
	cudaFree(pPixel);
}
