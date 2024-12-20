#pragma once

#include <immintrin.h>

inline float RemapValue(
	const float value,
	const float currMin,
	const float currMax,
	const float newMin,
	const float newMax
)
{
	return newMin + ((value - currMin) / (currMax - currMin)) * (newMax - newMin);
}

inline int ToIndex(const int x, const int y, const int width)
{
	return y * width + x;
}

inline __m256 powfAVX(const __m256 base, const __m256 exp)
{
	const __m256 logBase = _mm256_log_ps(base);
	const __m256 expLnBase = _mm256_mul_ps(exp, logBase);

	return _mm256_exp_ps(expLnBase);
}