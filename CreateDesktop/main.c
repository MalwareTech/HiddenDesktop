#include <Windows.h>
#include <stdio.h>

HDESK CreateHiddenDesktop(const char *desktop_name, HDESK *default_desktop)
{
    HDESK hidden_desktop = NULL, original_desktop;
    STARTUPINFOA si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    hidden_desktop = CreateDesktopA(desktop_name, NULL, NULL, 0, GENERIC_ALL, NULL);
    if (!hidden_desktop) {
        printf("CreateDesktopA() failed, error: %d\n", GetLastError());
        return NULL;
    }

    original_desktop = GetThreadDesktop(GetCurrentThreadId());
    if (!original_desktop) {
        printf("GetThreadDesktop() failed, error: %d\n", GetLastError());
        goto cleanup;
    }

    if (!SetThreadDesktop(hidden_desktop)) {
        printf("SetThreadDesktop() failed, error: %d\n", GetLastError());
        goto cleanup;
    }

    si.cb = sizeof(si);
    si.lpDesktop = (char *)desktop_name;

    if (!CreateProcessA("C:\\windows\\explorer.exe", NULL, NULL,
                        NULL, FALSE,  0, NULL,
                        NULL, &si, &pi)) {
        printf("CreateProcessA() failed, error: %d\n", GetLastError());
        goto cleanup;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetThreadDesktop(original_desktop);

    *default_desktop = original_desktop;

    return hidden_desktop;

cleanup:
    if(hidden_desktop)
        CloseDesktop(hidden_desktop);

    return FALSE;
}

void main() {
    HDESK hidden_desktop, original_desktop;
    MSG msg = { 0 };

    hidden_desktop = CreateHiddenDesktop("TestHiddenDesktop123", &original_desktop);
    if (!hidden_desktop)
        return;

    if (!SetThreadDesktop(hidden_desktop)) {
        printf("SetThreadDesktop() failed, error: %d\n", GetLastError());
        goto cleanup;
    }

    if (!RegisterHotKey(NULL, 1, MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'E')) {
        printf("RegisterHotKey() failed, error: %d\n", GetLastError());
        goto cleanup;
    }

    printf("Use CTRL + ALT + E to switch back to default desktop.\n");
    printf("Hit enter to switch into hidden desktop\n");
    
    getchar();
    
    SwitchDesktop(hidden_desktop);

    while (GetMessage(&msg, NULL, 0, 0) != 0)
    {
        if (msg.message == WM_HOTKEY)
        {
            printf("Exited hidden desktop\n");
            SwitchDesktop(original_desktop);
            break;
        }
    }

cleanup:
    if(hidden_desktop)
        CloseDesktop(hidden_desktop);
}
