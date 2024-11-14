#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "ProcessingHelperGPU.h"

#include <stdio.h>


static __global__ void Test()
{
	printf("Test");
}

void CallTest()
{
	Test<<<1, 10, 10 >>>();
}
