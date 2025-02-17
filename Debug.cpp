#include "Debug.h"

void GetErrorDescription(const HRESULT hr, char* msgBuffer)
{
    HRESULT code = hr;
    if (FACILITY_WINDOWS == HRESULT_FACILITY(code))
    {
        code = HRESULT_CODE(hr);
    }

    TCHAR* errMsg;
    TCHAR buffer[EDebugConstant::DEFAULT_BUFFER_SIZE];

    if (FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        code,
        MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
        reinterpret_cast<LPTSTR>(&errMsg),
        0,
        nullptr) != 0)
    {
        _stprintf(buffer, TEXT("%s"), errMsg);
        LocalFree(errMsg);
    }
    else
    {
        _stprintf(buffer, TEXT("[Could not find a description for error # %#x.]\n"), hr);
    }

    wcstombs(msgBuffer, buffer, wcslen(buffer) + 1);
}
