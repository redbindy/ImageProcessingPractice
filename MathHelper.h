#pragma once

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