#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include "MinHook.h"

static HMODULE real_winhttp = NULL;

/* -------------------------
   Logger
   ------------------------- */

static void log_line(const char *msg)
{
    FILE *f = fopen("C:\\hunter_winhttp_proxy.log", "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

/* -------------------------
   Hook targets
   ------------------------- */

typedef LRESULT (WINAPI *DispatchMessageW_t)(const MSG *);
typedef LRESULT (WINAPI *DispatchMessageA_t)(const MSG *);
typedef BOOL (WINAPI *TrackPopupMenu_t)(HMENU, UINT, int, int, int, HWND, const RECT *);
typedef BOOL (WINAPI *TrackPopupMenuEx_t)(HMENU, UINT, int, int, HWND, LPTPMPARAMS);

static DispatchMessageW_t real_DispatchMessageW;
static DispatchMessageA_t real_DispatchMessageA;
static TrackPopupMenu_t real_TrackPopupMenu;
static TrackPopupMenuEx_t real_TrackPopupMenuEx;

LRESULT WINAPI Hooked_DispatchMessageW(const MSG *msg)
{
    if (msg && msg->message == WM_CONTEXTMENU) {
        log_line("Blocked WM_CONTEXTMENU W");
        return 0;
    }
    return real_DispatchMessageW(msg);
}

LRESULT WINAPI Hooked_DispatchMessageA(const MSG *msg)
{
    if (msg && msg->message == WM_CONTEXTMENU) {
        log_line("Blocked WM_CONTEXTMENU A");
        return 0;
    }
    return real_DispatchMessageA(msg);
}

BOOL WINAPI Hooked_TrackPopupMenu(HMENU hMenu, UINT flags, int x, int y, int r, HWND w, const RECT *rc)
{
    log_line("Blocked TrackPopupMenu");
    return FALSE;
}

BOOL WINAPI Hooked_TrackPopupMenuEx(HMENU hMenu, UINT flags, int x, int y, HWND w, LPTPMPARAMS p)
{
    log_line("Blocked TrackPopupMenuEx");
    return FALSE;
}

/* -------------------------
   Install hooks
   ------------------------- */

static void install_hooks(void)
{
    MH_Initialize();

    MH_CreateHook(&DispatchMessageW, Hooked_DispatchMessageW, (LPVOID*)&real_DispatchMessageW);
    MH_CreateHook(&DispatchMessageA, Hooked_DispatchMessageA, (LPVOID*)&real_DispatchMessageA);
    MH_CreateHook(&TrackPopupMenu, Hooked_TrackPopupMenu, (LPVOID*)&real_TrackPopupMenu);
    MH_CreateHook(&TrackPopupMenuEx, Hooked_TrackPopupMenuEx, (LPVOID*)&real_TrackPopupMenuEx);

    MH_EnableHook(MH_ALL_HOOKS);

    log_line("Hooks installed");
}

/* -------------------------
   Load real winhttp
   ------------------------- */

static FARPROC get_real_proc(const char *name)
{
    if (!real_winhttp) {
        char path[MAX_PATH];
        GetSystemDirectoryA(path, MAX_PATH);
        lstrcatA(path, "\\winhttp.dll");

        real_winhttp = LoadLibraryA(path);
        log_line("Loaded real winhttp");
    }

    return GetProcAddress(real_winhttp, name);
}

/* -------------------------
   Forwarders
   ------------------------- */

#define FWD(name, ret, sig, args) \
__declspec(dllexport) ret WINAPI name sig { \
    typedef ret (WINAPI *fn_t) sig; \
    fn_t fn = (fn_t)get_real_proc(#name); \
    return fn ? fn args : (ret)0; \
}

FWD(WinHttpOpen, HINTERNET,
    (LPCWSTR a,DWORD b,LPCWSTR c,LPCWSTR d,DWORD e),
    (a,b,c,d,e))

FWD(WinHttpConnect, HINTERNET,
    (HINTERNET a,LPCWSTR b,INTERNET_PORT c,DWORD d),
    (a,b,c,d))

FWD(WinHttpOpenRequest, HINTERNET,
    (HINTERNET a,LPCWSTR b,LPCWSTR c,LPCWSTR d,LPCWSTR e,LPCWSTR *f,DWORD g),
    (a,b,c,d,e,f,g))

FWD(WinHttpSendRequest, BOOL,
    (HINTERNET a,LPCWSTR b,DWORD c,LPVOID d,DWORD e,DWORD f,DWORD_PTR g),
    (a,b,c,d,e,f,g))

FWD(WinHttpReceiveResponse, BOOL,
    (HINTERNET a,LPVOID b),
    (a,b))

FWD(WinHttpReadData, BOOL,
    (HINTERNET a,LPVOID b,DWORD c,LPDWORD d),
    (a,b,c,d))

FWD(WinHttpQueryHeaders, BOOL,
    (HINTERNET a,DWORD b,LPCWSTR c,LPVOID d,LPDWORD e,LPDWORD f),
    (a,b,c,d,e,f))

FWD(WinHttpCloseHandle, BOOL,
    (HINTERNET a),
    (a))

FWD(WinHttpAddRequestHeaders, BOOL,
    (HINTERNET a,LPCWSTR b,DWORD c,DWORD d),
    (a,b,c,d))

FWD(WinHttpSetStatusCallback, WINHTTP_STATUS_CALLBACK,
    (HINTERNET a,WINHTTP_STATUS_CALLBACK b,DWORD c,DWORD_PTR d),
    (a,b,c,d))

FWD(WinHttpCheckPlatform, BOOL,
    (void),
    ())

FWD(WinHttpGetIEProxyConfigForCurrentUser, BOOL,
    (WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *a),
    (a))

FWD(WinHttpSetTimeouts, BOOL,
    (HINTERNET a,int b,int c,int d,int e),
    (a,b,c,d,e))

FWD(WinHttpQueryDataAvailable, BOOL,
    (HINTERNET a,LPDWORD b),
    (a,b))

/* -------------------------
   Entry
   ------------------------- */

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID r)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        log_line("Proxy loaded");
        install_hooks();
    }

    if (reason == DLL_PROCESS_DETACH) {
        MH_Uninitialize();
        if (real_winhttp)
            FreeLibrary(real_winhttp);
    }

    return TRUE;
}