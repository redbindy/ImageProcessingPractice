#include "ProcessingHelperGPU.h"

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>

__global__ void EqualizePixels(ImageDTOForGPU imageGPU)
{
	const int pixelCount = imageGPU.width * imageGPU.height;
	
	const unsigned int col = (blockDim.x * blockIdx.x) + threadIdx.x;
	if (col >= imageGPU.width)
	{
		return;
	}

	const unsigned int row = (blockDim.y * blockIdx.y) + threadIdx.y;
	if (row >= imageGPU.height)
	{
		return;
	}

	const unsigned int index = row * imageGPU.width + col;

	__shared__ float cdf[TABLE_SIZE];

	if (threadIdx.x == 0 && threadIdx.y == 0)
	{
		uint32_t sum = 0;
		for (int i = 0; i < TABLE_SIZE; ++i)
		{
			switch (blockIdx.z)
			{
			case EHandleColor::RED:
				{
					sum += imageGPU.pFrequencyTable->redTable[i];
				}
				break;

			case EHandleColor::GREEN:
				{
					sum += imageGPU.pFrequencyTable->greenTable[i];
				}
				break;

			case EHandleColor::BLUE:
				{
					sum += imageGPU.pFrequencyTable->blueTable[i];
				}
				break;

			default:
				printf("Invalid input %d\n", blockIdx.z);
				break;
			}

			cdf[i] = sum / (float)pixelCount;
		}
	}

	__syncthreads();

	Pixel* pPixel = imageGPU.pPixels + index;
	switch (blockIdx.z)
	{
	case EHandleColor::RED:
		{
			pPixel->rgba.r = (uint8_t)round(UINT8_MAX * cdf[pPixel->rgba.r]);
		}
		break;

	case EHandleColor::GREEN:
		{
			pPixel->rgba.g = (uint8_t)round(UINT8_MAX * cdf[pPixel->rgba.g]);
		}
		break;

	case EHandleColor::BLUE:
		{
			pPixel->rgba.b = (uint8_t)round(UINT8_MAX * cdf[pPixel->rgba.b]);
		}
		break;

	default:
		printf("Invalid input %d\n", blockIdx.z);
		break;
	}
}

__global__ void CalculateFrequencyTable(ImageDTOForGPU imageGPU)
{
	const int pixelCount = imageGPU.width * imageGPU.height;

	const unsigned int col = (blockDim.x * blockIdx.x) + threadIdx.x;
	if (col >= imageGPU.width)
	{
		return;
	}

	const unsigned int row = (blockDim.y * blockIdx.y) + threadIdx.y;
	if (row >= imageGPU.height)
	{
		return;
	}

	const unsigned int index = row * imageGPU.width + col;

	Pixel* pPixel = imageGPU.pPixels + index;
	switch (blockIdx.z)
	{
	case EHandleColor::RED:
		{
			atomicAdd(imageGPU.pFrequencyTable->redTable + pPixel->rgba.r, 1);
		}
		break;

	case EHandleColor::GREEN:
		{
			atomicAdd(imageGPU.pFrequencyTable->greenTable + pPixel->rgba.g, 1);
		}
		break;

	case EHandleColor::BLUE:
		{
			atomicAdd(imageGPU.pFrequencyTable->blueTable + pPixel->rgba.b, 1);
		}
		break;

	default:
		printf("Invalid input %d\n", blockIdx.z);
		break;
	}
}

void EqualizeHelperGPU(ImageDTOForGPU image)
{
	const int pixelCount = image.width * image.height;

	ImageDTOForGPU imageGPU = { nullptr, image.width, image.height, nullptr };
	
	cudaError_t errorCode = cudaMalloc(&(imageGPU.pPixels), sizeof(Pixel) * pixelCount);
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		return;
	}

	errorCode = cudaMalloc(&(imageGPU.pFrequencyTable), sizeof(FrequencyTable));
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		return;
	}

	{
		dim3 blockDim = { 32, 32, 1 };
		dim3 gridDim = {
			(unsigned int)ceil(imageGPU.width / (float)blockDim.x),
			(unsigned int)ceil(imageGPU.height / (float)blockDim.y),
			EHandleColor::COUNT
		};

		cudaMemcpy(imageGPU.pPixels, image.pPixels, pixelCount * sizeof(Pixel), cudaMemcpyHostToDevice);
		cudaMemcpy(imageGPU.pFrequencyTable, image.pFrequencyTable, sizeof(FrequencyTable), cudaMemcpyHostToDevice);
		{
			EqualizePixels << <gridDim, blockDim >> > (imageGPU);
		}
		cudaMemcpy(image.pPixels, imageGPU.pFrequencyTable, pixelCount * sizeof(Pixel), cudaMemcpyDeviceToHost);

		cudaMemset(imageGPU.pFrequencyTable, 0, sizeof(FrequencyTable));
		{
			CalculateFrequencyTable << <gridDim, blockDim >> > (imageGPU);
		}
		cudaMemcpy(image.pFrequencyTable, imageGPU.pFrequencyTable, sizeof(FrequencyTable), cudaMemcpyDeviceToHost);
	}
	cudaFree(imageGPU.pPixels);
	cudaFree(imageGPU.pFrequencyTable);
}