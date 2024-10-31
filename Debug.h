#pragma once

#include <iostream>
#include <Windows.h>

#if defined(_DEBUG) || defined(DEBUG)

#define ASSERT_DEFAULT(expr) \
	if (!(expr)) \
	{ \
		DebugBreak(); \
	} \

#define ASSERT_WITH_MSG(expr, msg) \
	if (!(expr)) \
	{ \
		PrintErrorMessage((msg), (-1)); \
		DebugBreak(); \
	} \

#define ASSERT_WITH_MSG_AND_CODE(expr, msg, code) \
	if (!(expr)) \
	{ \
		PrintErrorMessage((msg), (code)); \
		DebugBreak(); \
	} \

#define EXPAND(x) x
#define VA_GENERIC(_1, _2, _3, x, ...) x
#define ASSERT(...) EXPAND(VA_GENERIC(__VA_ARGS__, ASSERT_WITH_MSG_AND_CODE, ASSERT_WITH_MSG, ASSERT_DEFAULT)(__VA_ARGS__))

#endif

inline void PrintErrorMessage(const char* msg, const HRESULT hr)
{
	std::cerr << msg << '\n'
		<< std::showbase << std::hex << "Code: " << hr << std::endl;
}