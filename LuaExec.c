#include <windows.h>
#include <stdio.h>
#include <stdint.h> 
#include <wchar.h>
#define LUA_REGISTRYINDEX (-10000)
typedef struct lua_State lua_State;

// Typedef
typedef int  (*luaL_ref_Func)(lua_State* L, int t);
typedef void (*luaL_unref_Func)(lua_State* L, int t, int ref);
typedef lua_State* (*luaL_newstate_Func)(void);
typedef lua_State* (*lua_newthread_Func)(lua_State* L);
typedef void (*lua_settop_Func)(lua_State* L, int index);
typedef void (*luaL_openlibs_Func)(lua_State*);
typedef void (*lua_close_Func)(lua_State*);
typedef int  (*luaL_loadstring_Func)(lua_State*, const char*);
typedef int  (*lua_vpcall_Func)(lua_State*, int, int, int);

// Hàm Lua
static luaL_newstate_Func luaL_newstate = NULL;
static luaL_openlibs_Func luaL_openlibs = NULL;
static lua_close_Func lua_close = NULL;
static luaL_loadstring_Func luaL_loadstring = NULL;
static lua_vpcall_Func lua_vpcall = NULL;
static HMODULE g_hModule = NULL;
static WNDPROC oldEditProc = NULL;
static lua_newthread_Func lua_newthread = NULL;
static lua_settop_Func    lua_settop    = NULL;
static luaL_ref_Func   luaL_ref = NULL;
static luaL_unref_Func luaL_unref = NULL;
typedef struct {
    char *code;
    lua_State *L;
} LuaExecParam;

lua_State* CreateLuaThread(lua_State* L) {
    if (!L) return NULL;
    lua_State* thread = lua_newthread(L);
    return thread;
}
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
    LuaExecParam *p = (LuaExecParam*)param;
    if (!p || !p->L) {
        if (p) free(p->code), free(p);
        return 0;
    }

    lua_State *L = CreateLuaThread(p->L);
	if (!L) {
        free(p->code);
        free(p);
        return 0;
    }
	int ref = luaL_ref(p->L, LUA_REGISTRYINDEX);
    char *code = p->code;
    if (luaL_loadstring(L, code) == 0) {
        if (lua_vpcall(L, 0, 0, 0) == 0) {
            MessageBoxW(NULL, L"Lua code executed!", L"Success", MB_OK);
        } else {
            MessageBoxW(NULL, L"Lua error!", L"Error", MB_OK);
        }
    } else {
        MessageBoxW(NULL, L"Lua load error!", L"Error", MB_OK);
    }
	luaL_unref(p->L, LUA_REGISTRYINDEX, ref);
	free(code);
	free(p);
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
            free(wbuffer);
        wchar_t addrBuf[128];
        GetDlgItemTextW(hwndDlg, 1003, addrBuf, 128);

        if (wcslen(addrBuf) == 0) {
            MessageBoxW(hwndDlg, L"Chưa nhập địa chỉ lua_State!", L"Cảnh báo", MB_OK | MB_ICONWARNING);
            free(utf8buf);
            break;
        }

        wchar_t fullAddr[130] = L"0x";
        wcscat(fullAddr, addrBuf);

        wchar_t *endptr = NULL;
        uintptr_t addr = (uintptr_t)wcstoull(fullAddr, &endptr, 16);

        if (endptr == fullAddr + 2 || *endptr != L'\0') {
            MessageBoxW(hwndDlg, L"Địa chỉ không hợp lệ! Vui lòng nhập HEX.", L"Cảnh báo", MB_OK | MB_ICONWARNING);
            free(utf8buf);
            break;
        }

        lua_State *L = (lua_State*)addr;
        LuaExecParam *p = (LuaExecParam*)malloc(sizeof(LuaExecParam));
        if (!p) {
            MessageBoxW(hwndDlg, L"Không đủ bộ nhớ param!", L"Error", MB_OK);
            free(utf8buf);
            break;
        }
        p->code = utf8buf;
        p->L = L;
        CreateThread(NULL, 0, LuaThread, p, 0, NULL);
    }
    break;
    case WM_SIZE:
	{
HWND hLabel = GetDlgItem(hwndDlg, 1000);
HWND hEdit  = GetDlgItem(hwndDlg, 1001);
HWND hBtn   = GetDlgItem(hwndDlg, 1002);
HWND hEdit2 = GetDlgItem(hwndDlg, 1003); 

if (hEdit && hBtn && hEdit2) {
    RECT rcClient;
    GetClientRect(hwndDlg, &rcClient);

    int edit2Left   = 10;
    int edit2Bottom = rcClient.bottom - 10;
    RECT rcEdit2;
    GetWindowRect(hEdit2, &rcEdit2);
    int edit2Width  = rcEdit2.right - rcEdit2.left;
    int edit2Height = rcEdit2.bottom - rcEdit2.top;
    int edit2Top    = edit2Bottom - edit2Height;
    MoveWindow(hEdit2, edit2Left, edit2Top, edit2Width, edit2Height, TRUE);

    int editTop = 30;
    int editLeft = 10;
    int editRight = rcClient.right - 10;
    int editBottom = edit2Top - 5;
    MoveWindow(hEdit, editLeft, editTop, editRight - editLeft, editBottom - editTop, TRUE);

    int labelLeft = editLeft;
    int labelTop  = editTop - 20;
    int labelWidth = editRight - editLeft;
    int labelHeight = 20;
    MoveWindow(hLabel, labelLeft, labelTop, labelWidth, labelHeight, TRUE);

    int btnWidth = 80, btnHeight = 25;
    MoveWindow(hBtn, (rcClient.right - btnWidth) / 2,
                      rcClient.bottom - btnHeight - 10,
                      btnWidth, btnHeight, TRUE);
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

    // Hàm chạy lua
    luaL_newstate   = (luaL_newstate_Func)GetProcAddress(hLua, "luaL_newstate");
    luaL_openlibs   = (luaL_openlibs_Func)GetProcAddress(hLua, "luaL_openlibs");
    lua_close       = (lua_close_Func)GetProcAddress(hLua, "lua_close");
    luaL_loadstring = (luaL_loadstring_Func)GetProcAddress(hLua, "luaL_loadstring");
    lua_vpcall      = (lua_vpcall_Func)GetProcAddress(hLua, "lua_vpcall");
	lua_newthread = (lua_newthread_Func)GetProcAddress(hLua, "lua_newthread");
    lua_settop    = (lua_settop_Func)GetProcAddress(hLua, "lua_settop");
	luaL_ref   = (luaL_ref_Func)GetProcAddress(hLua, "luaL_ref");
    luaL_unref = (luaL_unref_Func)GetProcAddress(hLua, "luaL_unref");

    if (!luaL_newstate || !luaL_openlibs || !lua_close || !luaL_loadstring || !lua_vpcall || !lua_newthread || !lua_settop || !luaL_ref || !luaL_unref) {
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

