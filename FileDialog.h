#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <ShObjIdl.h>

#include "Debug.h"
#include "ComHelper.h"

enum EFileDialogConstant
{
    DEFAULT_PATH_LEN = 1024
};

class FileDialog final
{    
public:
    static void CreateInstance(const HWND hWnd);
    static FileDialog* GetInstance();
    static void DeleteInstance();

    bool TryOpenFileDialog(char outFilePath[], const int bufferLength);

private:
    static FileDialog* staticInstance;

    HWND mhOwnerWnd;
    IFileOpenDialog* mpOpenDialog;

private:
    FileDialog(const HWND hWnd);
    ~FileDialog();
    FileDialog(const FileDialog& other) = delete;
    FileDialog(FileDialog&& other) = delete;
    FileDialog& operator=(const FileDialog& other) = delete;
    FileDialog& operator=(FileDialog&& other) = delete;
};