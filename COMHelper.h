#pragma once

template<typename COM>
inline void SafeRelease(COM*& pCom)
{
	if (pCom != nullptr)
	{
		pCom->Release();
		pCom = nullptr;
	}
}