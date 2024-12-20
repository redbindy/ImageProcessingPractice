#pragma once

#include <Windows.h>
#include <vector>
#include <thread>

#include "Debug.h"
#include "TypeDef.h"
#include "Image.h"

void EqualizeHelperCPU(Image& outImage);
void GammaHelperCPU(Image& outImage, const float gamma);