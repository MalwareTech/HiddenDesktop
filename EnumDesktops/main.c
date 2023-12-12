#include <Windows.h>
#include <stdio.h>
#include <psapi.h>

BOOL GetProcessName(DWORD process_id, char *process_name, size_t name_length) {
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (process == NULL) {
        printf("OpenProcess() failed, error: %d\n", GetLastError());
        return FALSE;
    }

    if (!GetModuleBaseNameA(process, NULL, process_name, name_length)) {
        printf("GetModuleFileNameExA() failed, error: %d\n", GetLastError());
        CloseHandle(process);
        return FALSE;
    } 
    
    CloseHandle(process);
    return TRUE;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD process_id = NULL;
    HANDLE process;
    PDWORD process_ids = (PDWORD)lParam;
    char process_name[MAX_PATH];

    if (!GetWindowThreadProcessId(hwnd, &process_id)) {
        printf("GetWindowThreadProcessId() failed, error: %d\n", GetLastError());
        return FALSE;
    }

    for (int i = 0; i < 1024 && process_ids[i] != process_id; i++) {
        if (process_ids[i] == 0) {
            process_ids[i] = process_id;

            if (GetProcessName(process_id, process_name, MAX_PATH)) {
                printf("\tFound Process: %s\n", process_name);
            }

            break;
        }
    }

    return TRUE;
}

BOOL CALLBACK EnumDesktopProc(LPSTR lpszDesktop, LPARAM lParam) {
    DWORD desktop_pids[1024] = { 0 };

    if (_stricmp(lpszDesktop, "Default") != 0) {
        printf("Found Desktop: %s\n", lpszDesktop);

        HDESK desktop = OpenDesktopA(lpszDesktop, 0, FALSE, GENERIC_ALL);

        EnumDesktopWindows(desktop, EnumWindowsProc, (LPARAM)&desktop_pids);

        CloseDesktop(desktop);
    }

    return TRUE;
}

BOOL SwitchToDesktop(char* desktop_name) {
    BOOL success = FALSE;
    HDESK desktop = NULL; 

    desktop = OpenDesktopA(desktop_name, 0, FALSE, GENERIC_ALL);
    if (!desktop) {
        printf("OpenDesktopA() failed, error: %d\n", GetLastError());
        goto cleanup;
    }

    if (!SetThreadDesktop(desktop)) {
        printf("SetThreadDesktop() failed, error: %d\n", GetLastError());
        goto cleanup;
    }

    if (!SwitchDesktop(desktop)) {
        printf("SwitchDesktop() failed, error: %d\n", GetLastError());
        goto cleanup;
    }

    success = TRUE;

cleanup:
    if (desktop)
        CloseDesktop(desktop);

    return success;
}


void main() {
    char desktop_name[MAX_PATH];
    MSG msg = { 0 };

    EnumDesktopsA(GetProcessWindowStation(), EnumDesktopProc, NULL);

    if (!RegisterHotKey(NULL, 1, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'E')) {
        printf("RegisterHotKey('E') failed, error: %d\n", GetLastError());
        return;
    }

    HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
    if (!old_desktop) {
        printf("GetThreadDesktop() failed, error: %d\n", GetLastError());
        return;
    }

    while (TRUE) {
        printf("Note: Use CTRL + ALT + E to switch back to the default desktop\r\n");
        printf("Enter desktop to switch to: ");
        scanf_s("%s", desktop_name, MAX_PATH);
        
        if (SwitchToDesktop(desktop_name)) {
          
            while (GetMessage(&msg, NULL, 0, 0) != 0)
            {
                if (msg.message == WM_HOTKEY)
                {
                    printf("Exiting desktop %s\r\n", desktop_name);
                    SwitchDesktop(old_desktop);
                    break;
                }
            }
        }
    }
}
