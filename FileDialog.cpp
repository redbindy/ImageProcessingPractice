#include "FileDialog.h"

FileDialog* FileDialog::staticInstance = nullptr;

FileDialog::FileDialog(const HWND hWnd)
    : mhOwnerWnd(hWnd)
    , mpOpenDialog(nullptr)
{
    char msg[EDebugConstant::DEFAULT_BUFFER_SIZE];

    // com init
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    GetErrorDescription(hr, msg);
    ASSERT(SUCCEEDED(hr), msg);

    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&mpOpenDialog));

    GetErrorDescription(hr, msg);
    ASSERT(SUCCEEDED(hr), msg);

    COMDLG_FILTERSPEC filterSpecs[] =
    {
        { TEXT("Image"), TEXT("*.jpg;*.jpeg;*.png;*.gif;*.bmp") },
        { TEXT("All"), TEXT("*.*") }
    };

    mpOpenDialog->SetFileTypes(ARRAYSIZE(filterSpecs), filterSpecs);
}

FileDialog::~FileDialog()
{
    SafeRelease(mpOpenDialog);

    CoUninitialize();
}

void FileDialog::CreateInstance(const HWND hWnd)
{
    if (staticInstance != nullptr)
    {
        delete staticInstance;
    }

    staticInstance = new FileDialog(hWnd);
}

FileDialog* FileDialog::GetInstance()
{
    ASSERT(staticInstance != nullptr, "Should call CreateInstance() calling GetInstance()");

    return staticInstance;
}

void FileDialog::DeleteInstance()
{
    delete staticInstance;
}

bool FileDialog::TryOpenFileDialog(char outFilePath[], const int bufferLength)
{
    ASSERT(outFilePath != nullptr);
    ASSERT(bufferLength > 0);

    char buffer[EDebugConstant::DEFAULT_BUFFER_SIZE];

    HRESULT hr = mpOpenDialog->Show(mhOwnerWnd);
    if (SUCCEEDED(hr))
    {
        IShellItem* pItem = nullptr;
        hr = mpOpenDialog->GetResult(&pItem);

        if (SUCCEEDED(hr))
        {
            TCHAR* pFilePath = nullptr;
            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pFilePath);

            GetErrorDescription(hr, buffer);
            ASSERT(SUCCEEDED(hr), buffer);
            {
                const int filePathLength = static_cast<int>(wcslen(pFilePath));
                ASSERT(filePathLength + 1 <= bufferLength);

                wcstombs(outFilePath, pFilePath, wcslen(pFilePath) + 1);
            }
            CoTaskMemFree(pFilePath);
        }
        SafeRelease(pItem);
    }

    return SUCCEEDED(hr);
}