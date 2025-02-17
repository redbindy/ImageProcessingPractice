#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <Windows.h>
#include <tchar.h>

enum EDebugConstant
{
    DEFAULT_BUFFER_SIZE = 1024
};

#if defined(_DEBUG) || defined(DEBUG)

#define ASSERT_DEFAULT(expr) \
    if (!(expr))             \
    {                        \
        DebugBreak();        \
        std::terminate();    \
    }                        \

#define ASSERT_WITH_MSG(expr, msg)       \
    if (!(expr))                         \
    {                                    \
        std::cerr << (msg) << std::endl; \
        DebugBreak();                    \
        std::terminate();                \
    }                                    \

#define EXPAND(x) x
#define VA_GENERIC(_1, _2, x, ...) x
#define ASSERT(...) EXPAND(VA_GENERIC(__VA_ARGS__, ASSERT_WITH_MSG, ASSERT_DEFAULT)(__VA_ARGS__))

#else
#define ASSERT
#endif

void GetErrorDescription(const HRESULT hr, char* msgBuffer);

inline void GetHResultDataFromWin32(HRESULT& outHResult, char* msgBuffer)
{
    outHResult = HRESULT_FROM_WIN32(GetLastError());
    GetErrorDescription(outHResult, msgBuffer);
}