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
static WNDPROC oldEditProc = NULL;

// UI
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if ((uMsg == WM_KEYDOWN) && (GetKeyState(VK_CONTROL) & 0x8000) && (wParam == 'A')) {
        SendMessage(hwnd, EM_SETSEL, 0, -1);
        return 0;
    }
    return CallWindowProc(oldEditProc, hwnd, uMsg, wParam, lParam);
}
DWORD WINAPI LuaThread(LPVOID param) {
	// Lua state
    char *code = (char*)param;
    lua_State *L = luaL_newstate();
	if (!L) { free(code); return 0; };
    luaL_openlibs(L);

    if (luaL_loadstring(L, code) == 0) {
        if (lua_vpcall(L, 0, 0, 0) == 0) {
            MessageBoxW(NULL, L"Lua code executed!", L"Success", MB_OK);
        } else {
            MessageBoxW(NULL, L"Lua error!", L"Error", MB_OK);
        }
    } else {
        MessageBoxW(NULL, L"Lua load error!", L"Error", MB_OK);
    }
    lua_close(L);
	free(code);
    return 0;
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
		    size_t bufSize = 1024 * 1024;
			wchar_t *wbuffer = (wchar_t*)malloc(bufSize * sizeof(wchar_t));
			if (!wbuffer) {
				MessageBoxW(hwndDlg, L"Không đủ bộ nhớ!", L"Error", MB_OK);
				break;
			}
            GetDlgItemTextW(hwndDlg, 1001, wbuffer, (int)bufSize);
			
			int len = WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, NULL, 0, NULL, NULL);
			char *utf8buf = (char*)malloc(len);
			if (!utf8buf) {
				MessageBoxW(hwndDlg, L"Không đủ bộ nhớ UTF-8", L"Error", MB_OK);
				free(wbuffer);
				break;
			}
			WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, utf8buf, len, NULL, NULL);			
			CreateThread(NULL, 0, LuaThread, utf8buf, 0, NULL);
			free(wbuffer);
        }
        break;
    case WM_SIZE:
	{
		HWND hLabel = GetDlgItem(hwndDlg, 1000);
        HWND hEdit  = GetDlgItem(hwndDlg, 1001);
        HWND hBtn   = GetDlgItem(hwndDlg, 1002);
        if (hEdit && hBtn) {
            RECT rcClient;
            GetClientRect(hwndDlg, &rcClient);
			int editTop = 30;
            int editLeft = 10;
            int editRight = rcClient.right - 10;
            int editBottom = rcClient.bottom - 50;
			MoveWindow(hEdit, editLeft, editTop, editRight - editLeft, editBottom - editTop, TRUE);
			int labelLeft = editLeft;
			int labelTop  = editTop - 20;
			int labelWidth = editRight - editLeft;
			int labelHeight = 20;
			MoveWindow(hLabel, labelLeft, labelTop, labelWidth, labelHeight, TRUE);
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
        MessageBoxW(NULL, L"Không tìm thấy liblua.dll", L"DLL Inject", MB_OK);
        return 0;
    }

    // hàm chạy lua
    luaL_newstate   = (luaL_newstate_Func)GetProcAddress(hLua, "luaL_newstate");
    luaL_openlibs   = (luaL_openlibs_Func)GetProcAddress(hLua, "luaL_openlibs");
    lua_close       = (lua_close_Func)GetProcAddress(hLua, "lua_close");
    luaL_loadstring = (luaL_loadstring_Func)GetProcAddress(hLua, "luaL_loadstring");
    lua_vpcall      = (lua_vpcall_Func)GetProcAddress(hLua, "lua_vpcall");

    if (!luaL_newstate || !luaL_openlibs || !lua_close || !luaL_loadstring || !lua_vpcall) {
        MessageBoxW(NULL, L"Không lấy được hàm Lua", L"DLL Inject", MB_OK);
        return 0;
    }

    // Open UI
    DialogBoxParamW(g_hModule, MAKEINTRESOURCE(101), NULL, DlgProc, 0);
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

