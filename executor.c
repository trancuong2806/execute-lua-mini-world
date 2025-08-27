#include <windows.h>
#include <stdio.h>

typedef struct lua_State lua_State;

// Typedef
typedef lua_State* (*luaL_newstate_Func)(void);
typedef void (*luaL_openlibs_Func)(lua_State*);
typedef void (*lua_close_Func)(lua_State*);
typedef int  (*luaL_loadstring_Func)(lua_State*, const char*);
typedef int  (*lua_vpcall_Func)(lua_State*, int, int, int);

// hàm Lua
static luaL_newstate_Func luaL_newstate = NULL;
static luaL_openlibs_Func luaL_openlibs = NULL;
static lua_close_Func lua_close = NULL;
static luaL_loadstring_Func luaL_loadstring = NULL;
static lua_vpcall_Func lua_vpcall = NULL;
static HMODULE g_hModule = NULL;
static lua_State *gL = NULL;
static WNDPROC oldEditProc = NULL;

// UI
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if ((uMsg == WM_KEYDOWN) && (GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'A')) {
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return 0;
    }
    return CallWindowProc(oldEditProc, hwnd, uMsg, wParam, lParam);
}
INT_PTR CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
	case WM_INITDIALOG: {
		HWND hEdit = GetDlgItem(hwndDlg, 1001);
		if (hEdit)
			oldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
		return TRUE;
	}
    case WM_COMMAND:
        if (LOWORD(wParam) == 1002) { // button
            char buffer[1024];
            GetDlgItemTextA(hwndDlg, 1001, buffer, sizeof(buffer));

            if (luaL_loadstring(gL, buffer) == 0) {
                lua_vpcall(gL, 0, 0, 0);
                MessageBoxA(hwndDlg, "Lua code executed!", "Success", MB_OK);
            } else {
                MessageBoxA(hwndDlg, "Lua error!", "Error", MB_OK);
            }
        }
        break;
    case WM_SIZE:
	{
        HWND hEdit = GetDlgItem(hwndDlg, 1001);
        HWND hBtn  = GetDlgItem(hwndDlg, 1002);
        if (hEdit && hBtn) {
            RECT rcClient;
            GetClientRect(hwndDlg, &rcClient);
			int editTop = 30;
            int editLeft = 10;
            int editRight = rcClient.right - 10;
            int editBottom = rcClient.bottom - 50;
			MoveWindow(hEdit, editLeft, editTop, editRight - editLeft, editBottom - editTop, TRUE);
			int btnWidth = 80, btnHeight = 25;
            MoveWindow(hBtn, (rcClient.right - btnWidth) / 2, rcClient.bottom - btnHeight - 10, btnWidth, btnHeight, TRUE);
        }
	}
		break;
    case WM_CLOSE:
       
        return TRUE;
	
	default:
        return FALSE;
	
    }
    return FALSE;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    // module liblua.dll
    HMODULE hLua = GetModuleHandleA("liblua.dll");
    if (!hLua) {
        MessageBoxA(NULL, "Không tìm thấy liblua.dll", "DLL Inject", MB_OK);
        return 0;
    }

    // hàm chạy lua
    luaL_newstate   = (luaL_newstate_Func)GetProcAddress(hLua, "luaL_newstate");
    luaL_openlibs   = (luaL_openlibs_Func)GetProcAddress(hLua, "luaL_openlibs");
    lua_close       = (lua_close_Func)GetProcAddress(hLua, "lua_close");
    luaL_loadstring = (luaL_loadstring_Func)GetProcAddress(hLua, "luaL_loadstring");
    lua_vpcall      = (lua_vpcall_Func)GetProcAddress(hLua, "lua_vpcall");

    if (!luaL_newstate || !luaL_openlibs || !lua_close || !luaL_loadstring || !lua_vpcall) {
        MessageBoxA(NULL, "Không lấy được hàm Lua", "DLL Inject", MB_OK);
        return 0;
    }

    // Lua state
    gL = luaL_newstate();
    luaL_openlibs(gL);

    // Open UI
    extern HMODULE g_hModule;
    DialogBoxParamA(g_hModule, MAKEINTRESOURCE(101), NULL, DlgProc, 0);
    lua_close(gL);

    return 0;
}

// Entry point DLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
    }
    return TRUE;
}
