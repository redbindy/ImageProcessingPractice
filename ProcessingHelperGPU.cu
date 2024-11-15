#include "ProcessingHelperGPU.h"

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>

__global__ void EqualizeHelperComputePixel(
	Pixel* pPixelsGPU,
	const int imageWidth,
	const int imageHeight,
	FrequencyTable* pFrequencyTableGPU
)
{
	const int pixelCount = imageWidth * imageHeight;

	const unsigned int col = (blockDim.x * blockIdx.x) + threadIdx.x;
	if (col >= imageWidth)
	{
		return;
	}

	const unsigned int row = (blockDim.y * blockIdx.y) + threadIdx.y;
	if (row >= imageHeight)
	{
		return;
	}

	const unsigned int index = row * imageWidth + col;

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
					sum += pFrequencyTableGPU->redTable[i];
				}
				break;

			case EHandleColor::GREEN:
				{
					sum += pFrequencyTableGPU->greenTable[i];
				}
				break;

			case EHandleColor::BLUE:
				{
					sum += pFrequencyTableGPU->blueTable[i];
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

	Pixel* pPixel = pPixelsGPU + index;
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

__global__ void EqualizeHelperGetFrequencyTable(
	Pixel* pPixelsGPU,
	const int imageWidth,
	const int imageHeight,
	FrequencyTable* pFrequencyTableGPU
)
{
	const int pixelCount = imageWidth * imageHeight;

	const unsigned int col = (blockDim.x * blockIdx.x) + threadIdx.x;
	if (col >= imageWidth)
	{
		return;
	}

	const unsigned int row = (blockDim.y * blockIdx.y) + threadIdx.y;
	if (row >= imageHeight)
	{
		return;
	}

	const unsigned int index = row * imageWidth + col;

	Pixel* pPixel = pPixelsGPU + index;
	switch (blockIdx.z)
	{
	case EHandleColor::RED:
		{
			atomicAdd(pFrequencyTableGPU->redTable + pPixel->rgba.r, 1);
		}
		break;

	case EHandleColor::GREEN:
		{
			atomicAdd(pFrequencyTableGPU->greenTable + pPixel->rgba.g, 1);
		}
		break;

	case EHandleColor::BLUE:
		{
			atomicAdd(pFrequencyTableGPU->blueTable + pPixel->rgba.b, 1);
		}
		break;

	default:
		printf("Invalid input %d\n", blockIdx.z);
		break;
	}
}

void EqualizeHelperGPU(
	Pixel* pPixels,
	const int imageWidth,
	const int imageHeight,
	FrequencyTable* pFrequencyTable
)
{
	const int pixelCount = imageWidth * imageHeight;

	Pixel* pPixelsGPU = nullptr;
	FrequencyTable* pFrequencyTableGPU = nullptr;

	cudaError_t errorCode = cudaMalloc(&pPixelsGPU, sizeof(Pixel) * pixelCount);
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		return;
	}

	errorCode = cudaMalloc(&pFrequencyTableGPU, sizeof(FrequencyTable));
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		return;
	}

	{
		dim3 blockDim = { 32, 32, 1 };
		dim3 gridDim = {
			(unsigned int)ceil(imageWidth / (float)blockDim.x),
			(unsigned int)ceil(imageHeight / (float)blockDim.y),
			EHandleColor::COUNT
		};

		cudaMemcpy(pPixelsGPU, pPixels, pixelCount * sizeof(Pixel), cudaMemcpyHostToDevice);
		cudaMemcpy(pFrequencyTableGPU, pFrequencyTable, sizeof(FrequencyTable), cudaMemcpyHostToDevice);
		{
			EqualizeHelperComputePixel << <gridDim, blockDim >> > (pPixelsGPU, imageWidth, imageHeight, pFrequencyTableGPU);
		}
		cudaMemcpy(pPixels, pPixelsGPU, pixelCount * sizeof(Pixel), cudaMemcpyDeviceToHost);
		cudaMemset(pFrequencyTableGPU, 0, sizeof(FrequencyTable));
		{
			EqualizeHelperGetFrequencyTable << <gridDim, blockDim >> > (pPixelsGPU, imageWidth, imageHeight, pFrequencyTableGPU);
		}
		cudaMemcpy(pFrequencyTable, pFrequencyTableGPU, sizeof(FrequencyTable), cudaMemcpyDeviceToHost);
	}
	cudaFree(pPixelsGPU);
	cudaFree(pFrequencyTableGPU);
}