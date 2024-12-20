#include "ProcessingHelperGPU.h"

#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>

__global__ void EqualizePixels(ImageDTOForGPU image)
{
	assert(image.pPixels != nullptr);
	assert(image.pFrequencyTable != nullptr);

	const int pixelCount = image.width * image.height;

	const unsigned int col = (blockDim.x * blockIdx.x) + threadIdx.x;
	if (col >= image.width)
	{
		return;
	}

	const unsigned int row = (blockDim.y * blockIdx.y) + threadIdx.y;
	if (row >= image.height)
	{
		return;
	}

	const unsigned int index = row * image.width + col;

	__shared__ float cdf[TABLE_SIZE];

	const int k = threadIdx.y * blockDim.x + threadIdx.x;
	if (k < TABLE_SIZE)
	{
		uint32_t sum = 0;
		for (int i = 0; i <= k; ++i)
		{
			switch (blockIdx.z)
			{
			case EHandleColor::RED:
				{
					sum += image.pFrequencyTable->redTable[i];
				}
				break;

			case EHandleColor::GREEN:
				{
					sum += image.pFrequencyTable->greenTable[i];
				}
				break;

			case EHandleColor::BLUE:
				{
					sum += image.pFrequencyTable->blueTable[i];
				}
				break;

			default:
				printf("Invalid input %d\n", blockIdx.z);
				break;
			}
		}

		cdf[k] = sum / (float)pixelCount;
	}

	__syncthreads();

	Pixel* pPixel = image.pPixels + index;
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
		assert(false);
		break;
	}
}

__global__ void CalculateFrequencyTable(ImageDTOForGPU image)
{
	assert(image.pPixels != nullptr);
	assert(image.pFrequencyTable != nullptr);

	assert(image.pPixels != nullptr);
	assert(image.pFrequencyTable != nullptr);

	const unsigned int col = (blockDim.x * blockIdx.x) + threadIdx.x;
	if (col >= image.width)
	{
		return;
	}

	const unsigned int row = (blockDim.y * blockIdx.y) + threadIdx.y;
	if (row >= image.height)
	{
		return;
	}

	const unsigned int index = row * image.width + col;

	Pixel* pPixel = image.pPixels + index;
	switch (blockIdx.z)
	{
	case EHandleColor::RED:
		{
			atomicAdd(image.pFrequencyTable->redTable + pPixel->rgba.r, 1);
		}
		break;

	case EHandleColor::GREEN:
		{
			atomicAdd(image.pFrequencyTable->greenTable + pPixel->rgba.g, 1);
		}
		break;

	case EHandleColor::BLUE:
		{
			atomicAdd(image.pFrequencyTable->blueTable + pPixel->rgba.b, 1);
		}
		break;

	default:
		assert(false);
		break;
	}
}

void EqualizeHelperGPU(ImageDTOForGPU image)
{
	assert(image.pPixels != nullptr);
	assert(image.pFrequencyTable != nullptr);

	const int pixelCount = image.width * image.height;

	ImageDTOForGPU imageGPU = { nullptr, image.width, image.height, nullptr };

	cudaError_t errorCode = cudaMalloc(&(imageGPU.pPixels), sizeof(Pixel) * pixelCount);
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		goto CUDA_FREE;
	}

	errorCode = cudaMalloc(&(imageGPU.pFrequencyTable), sizeof(FrequencyTable));
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		goto CUDA_FREE;
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
CUDA_FREE:;
	cudaFree(imageGPU.pPixels);
	cudaFree(imageGPU.pFrequencyTable);
}

__global__ void Match(ImageDTOForGPU outImage, ImageDTOForGPU srcImage, ImageDTOForGPU refImage)
{
	assert(srcImage.pPixels != nullptr);
	assert(srcImage.pFrequencyTable != nullptr);
	assert(refImage.pPixels != nullptr);
	assert(refImage.pFrequencyTable != nullptr);
	assert(outImage.pPixels != nullptr);
	assert(outImage.pFrequencyTable != nullptr);

	const unsigned int col = (blockDim.x * blockIdx.x) + threadIdx.x;
	if (col >= outImage.width)
	{
		return;
	}

	const unsigned int row = (blockDim.y * blockIdx.y) + threadIdx.y;
	if (row >= outImage.height)
	{
		return;
	}

	const unsigned int index = row * outImage.width + col;

	__shared__ FrequencyTable lookup;

	const int i = threadIdx.y * blockDim.x + threadIdx.x;

	if (i < TABLE_SIZE)
	{
		int k;
		switch (blockIdx.z)
		{
		case EHandleColor::RED:
			{
				for (k = 0; k < TABLE_SIZE; ++k)
				{
					if (refImage.pFrequencyTable->redTable[k] > srcImage.pFrequencyTable->redTable[i])
					{
						break;
					}
				}

				lookup.redTable[i] = k;
			}
			break;

		case EHandleColor::GREEN:
			{
				for (k = 0; k < TABLE_SIZE; ++k)
				{
					if (refImage.pFrequencyTable->greenTable[k] > srcImage.pFrequencyTable->greenTable[i])
					{
						break;
					}
				}

				lookup.greenTable[i] = k;
			}
			break;

		case EHandleColor::BLUE:
			{
				for (k = 0; k < TABLE_SIZE; ++k)
				{
					if (refImage.pFrequencyTable->blueTable[k] > srcImage.pFrequencyTable->blueTable[i])
					{
						break;
					}
				}
				lookup.blueTable[i] = k;
			}
			break;

		default:
			assert(false);
			break;
		}
	}

	__syncthreads();

	Pixel* pOutPixel = outImage.pPixels + index;
	const Pixel srcPixel = srcImage.pPixels[index];
	switch (blockIdx.z)
	{
	case EHandleColor::RED:
		{
			pOutPixel->rgba.r = lookup.redTable[srcPixel.rgba.r];
		}
		break;

	case EHandleColor::GREEN:
		{
			pOutPixel->rgba.g = lookup.greenTable[srcPixel.rgba.g];
		}
		break;

	case EHandleColor::BLUE:
		{
			pOutPixel->rgba.b = lookup.blueTable[srcPixel.rgba.b];
		}
		break;

	default:
		assert(false);
		break;
	}

	pOutPixel->rgba.a = UINT8_MAX;
}

void MatchHelperGPU(ImageDTOForGPU outImage, ImageDTOForGPU srcImage, ImageDTOForGPU refImage)
{
	ImageDTOForGPU srcImageGPU = { nullptr, srcImage.width, srcImage.height, nullptr };
	ImageDTOForGPU refImageGPU = { nullptr, refImage.width, refImage.height, nullptr };
	ImageDTOForGPU outImageGPU = { nullptr, srcImage.width, srcImage.height, nullptr };

	cudaError_t errorCode = cudaMalloc(&(srcImageGPU.pPixels), sizeof(Pixel) * srcImage.width * srcImage.height);
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		goto CUDA_FREE;
	}

	errorCode = cudaMalloc(&(srcImageGPU.pFrequencyTable), sizeof(FrequencyTable));
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		goto CUDA_FREE;
	}

	errorCode = cudaMalloc(&(refImageGPU.pPixels), sizeof(Pixel) * refImage.width * refImage.height);
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		goto CUDA_FREE;
	}

	errorCode = cudaMalloc(&(refImageGPU.pFrequencyTable), sizeof(FrequencyTable));
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		goto CUDA_FREE;
	}

	errorCode = cudaMalloc(&(outImageGPU.pPixels), sizeof(Pixel) * outImage.width * outImage.height);
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		goto CUDA_FREE;
	}

	errorCode = cudaMalloc(&(outImageGPU.pFrequencyTable), sizeof(FrequencyTable));
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		goto CUDA_FREE;
	}

	{
		dim3 blockDim = { 32, 32, 1 };
		dim3 gridDim = {
			(unsigned int)ceil(srcImageGPU.width / (float)blockDim.x),
			(unsigned int)ceil(srcImageGPU.height / (float)blockDim.y),
			EHandleColor::COUNT
		};

		cudaMemcpy(srcImageGPU.pPixels, srcImage.pPixels, sizeof(Pixel) * srcImage.width * srcImage.height, cudaMemcpyHostToDevice);
		cudaMemcpy(srcImageGPU.pFrequencyTable, srcImage.pFrequencyTable, sizeof(FrequencyTable), cudaMemcpyHostToDevice);

		EqualizePixels << <gridDim, blockDim >> > (srcImageGPU);

		cudaMemset(srcImageGPU.pFrequencyTable, 0, sizeof(FrequencyTable));
		CalculateFrequencyTable << <gridDim, blockDim >> > (srcImageGPU);

		cudaMemcpy(refImageGPU.pPixels, refImage.pPixels, sizeof(Pixel) * refImage.width * refImage.height, cudaMemcpyHostToDevice);
		cudaMemcpy(refImageGPU.pFrequencyTable, refImage.pFrequencyTable, sizeof(FrequencyTable), cudaMemcpyHostToDevice);

		EqualizePixels << <gridDim, blockDim >> > (refImageGPU);

		cudaMemset(refImageGPU.pFrequencyTable, 0, sizeof(FrequencyTable));
		CalculateFrequencyTable << <gridDim, blockDim >> > (refImageGPU);

		Match << <gridDim, blockDim >> > (outImageGPU, srcImageGPU, refImageGPU);

		cudaMemset(outImageGPU.pFrequencyTable, 0, sizeof(FrequencyTable));
		CalculateFrequencyTable << <gridDim, blockDim >> > (outImageGPU);

		cudaMemcpy(outImage.pPixels, outImageGPU.pPixels, sizeof(Pixel) * outImage.width * outImage.height, cudaMemcpyDeviceToHost);
		cudaMemcpy(outImage.pFrequencyTable, outImageGPU.pFrequencyTable, sizeof(FrequencyTable), cudaMemcpyDeviceToHost);
	}
CUDA_FREE:;
	cudaFree(srcImageGPU.pPixels);
	cudaFree(srcImageGPU.pFrequencyTable);

	cudaFree(refImageGPU.pPixels);
	cudaFree(refImageGPU.pFrequencyTable);

	cudaFree(outImageGPU.pPixels);
	cudaFree(outImageGPU.pFrequencyTable);
}

__global__ void CorrectGamma(ImageDTOForGPU outImage, const float gamma)
{
	assert(outImage.pPixels != nullptr);

	const unsigned int col = (blockDim.x * blockIdx.x) + threadIdx.x;
	if (col >= outImage.width)
	{
		return;
	}

	const unsigned int row = (blockDim.y * blockIdx.y) + threadIdx.y;
	if (row >= outImage.height)
	{
		return;
	}

	constexpr float NORMALIZER = 1 / 255.f;
	constexpr float UNNORMALIZER = 255.f;

	const unsigned int index = row * outImage.width + col;

	Pixel* outPixel = outImage.pPixels + index;
	switch (blockIdx.z)
	{
	case EHandleColor::RED:
		float newR = powf(outPixel->rgba.r * NORMALIZER, gamma);
		newR *= UNNORMALIZER;
		outPixel->rgba.r = static_cast<uint8_t>(roundf(newR));
		break;

	case EHandleColor::GREEN:
		float newG = powf(outPixel->rgba.g * NORMALIZER, gamma);
		newG *= UNNORMALIZER;
		outPixel->rgba.g = static_cast<uint8_t>(roundf(newG));
		break;

	case EHandleColor::BLUE:
		float newB = powf(outPixel->rgba.b * NORMALIZER, gamma);
		newB *= UNNORMALIZER;
		outPixel->rgba.b = static_cast<uint8_t>(roundf(newB));
		break;

	default:
		assert(false);
		break;
	}
}

void GammaHelperGPU(ImageDTOForGPU image, const float gamma)
{
	assert(image.pPixels != nullptr);
	assert(image.pFrequencyTable != nullptr);

	const int pixelCount = image.width * image.height;

	ImageDTOForGPU imageGPU = { nullptr, image.width, image.height, nullptr };

	cudaError_t errorCode = cudaMalloc(&(imageGPU.pPixels), sizeof(Pixel) * pixelCount);
	if (errorCode != cudaSuccess)
	{
		printf("%s - %s\n", cudaGetErrorName(errorCode), cudaGetErrorString(errorCode));
		goto CUDA_FREE;
	}
	{
		dim3 blockDim = { 32, 32, 1 };
		dim3 gridDim = {
			(unsigned int)ceil(imageGPU.width / (float)blockDim.x),
			(unsigned int)ceil(imageGPU.height / (float)blockDim.y),
			EHandleColor::COUNT
		};

		cudaMemcpy(imageGPU.pPixels, image.pPixels, pixelCount * sizeof(Pixel), cudaMemcpyHostToDevice);
		{
			CorrectGamma << <gridDim, blockDim >> > (imageGPU, gamma);
		}
		cudaMemcpy(image.pPixels, imageGPU.pPixels, pixelCount * sizeof(Pixel), cudaMemcpyDeviceToHost);
	}
CUDA_FREE:;
	cudaFree(imageGPU.pPixels);
}
