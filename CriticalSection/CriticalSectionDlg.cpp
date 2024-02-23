#include "CriticalSectionDlg.h"
#include <string>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

CriticalSectionDlg* CriticalSectionDlg::ptr = nullptr;
CRITICAL_SECTION cs;
TCHAR filename[40] = L"array.txt";
CriticalSectionDlg::CriticalSectionDlg(void) {
    ptr = this;
}

CriticalSectionDlg::~CriticalSectionDlg(void) {
    DeleteCriticalSection(&cs);
}

void CriticalSectionDlg::Cls_OnClose(HWND hwnd) {
    EndDialog(hwnd, 0);
}

BOOL CriticalSectionDlg::Cls_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
    InitializeCriticalSection(&cs);
    ofstream out(filename);
    out << "Hello World!";
    return TRUE;
}

void MessageAboutError(DWORD dwError) {
    LPVOID lpMsgBuf = nullptr;
    TCHAR szBuf[300];

    BOOL fOK = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        nullptr, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0,
        nullptr);
    if (lpMsgBuf != nullptr) {
        wsprintf(szBuf, TEXT("Ошибка %d: %s"), dwError, lpMsgBuf);
        MessageBox(0, szBuf, TEXT("Сообщение об ошибке"), MB_OK | MB_ICONSTOP);
        LocalFree(lpMsgBuf);
    }
}

void CopyFileNTimes(LPCTSTR filename, int N) {
    ifstream sourceFile(filename);
    if (!sourceFile.is_open()) {
        MessageAboutError(GetLastError());
        return;
    }

    string line;
    if (!getline(sourceFile, line)) {
        sourceFile.close();
        return;
    }
    sourceFile.close();

    for (int i = 1; i <= N; ++i) {
        TCHAR newFilename[MAX_PATH];
        _stprintf_s(newFilename, _T("%s_%d.txt"), filename, i);

        ofstream destFile(newFilename);
        if (!destFile.is_open()) {
            MessageAboutError(GetLastError());
            continue;
        }

        destFile << line;
    }
}

DWORD WINAPI WriteToFile(LPVOID lp) {
    EnterCriticalSection(&cs);

    ofstream out(filename, ios::app);
    if (!out.is_open()) {
        MessageAboutError(GetLastError());
        LeaveCriticalSection(&cs);
        return 1;
    }


    int N = 5;
    CopyFileNTimes(filename, N);


    out.close();

    LeaveCriticalSection(&cs);

    return 0;
}

DWORD WINAPI ReadFromFile(LPVOID lp) {
    EnterCriticalSection(&cs);

    fs::path currentDir = fs::current_path();
    string substr = "array.";
    string line;
    ofstream destFile(filename); 

    for (const auto& entry : filesystem::directory_iterator(currentDir)) {
        if (entry.path().filename().string().find(substr) != string::npos) {
            ifstream file(entry.path());
            if (file.is_open()) {
                while (getline(file, line)) { 
                    destFile << line << "\n";
                }
                file.close();
            }
        }
    }

    destFile.close(); 

    LeaveCriticalSection(&cs);

    return 0;
}


void CriticalSectionDlg::Cls_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
    if (id == IDC_BUTTON1) {
        HANDLE hThread = CreateThread(nullptr, 0, WriteToFile, nullptr, 0, nullptr);
        CloseHandle(hThread);
        MessageBox(0, TEXT("Поток копирования отработал"), TEXT("Критическая секция"), MB_OK);

        hThread = CreateThread(nullptr, 0, ReadFromFile, nullptr, 0, nullptr);
        CloseHandle(hThread);
        MessageBox(0, TEXT("Поток записи отработал"), TEXT("Критическая секция"), MB_OK);

    }
}

BOOL CALLBACK CriticalSectionDlg::DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        HANDLE_MSG(hwnd, WM_CLOSE, ptr->Cls_OnClose);
        HANDLE_MSG(hwnd, WM_INITDIALOG, ptr->Cls_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, ptr->Cls_OnCommand);
    }
    return FALSE;
}
