#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include "memory.h"

uintptr_t GetModuleBaseAddress(DWORD processId, const wchar_t* ModuleTarget) {
	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
	if (snapshotHandle == INVALID_HANDLE_VALUE) {
		return NULL;
	}
	MODULEENTRY32W moduleEntry = { };
	moduleEntry.dwSize = sizeof(MODULEENTRY32W);
	if (Module32FirstW(snapshotHandle, &moduleEntry)) {
		do {
			if (_wcsicmp(moduleEntry.szModule, ModuleTarget) == 0) {
				CloseHandle(snapshotHandle);
				return reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
			}
		} while (Module32NextW(snapshotHandle, &moduleEntry));
	}
	CloseHandle(snapshotHandle);
	return NULL;
}

int width = 1026;
int height = 751;
float save; //view matrix icin save
int health = 11;
int value;
float x, y, z;
DWORD dusmanlarinclass, targetclass;
DWORD procID = 2968;
DWORD BaseModule = GetModuleBaseAddress(procID, L"ac_client.exe"); //ikinci degiskene, cheatenginedeki yesil adi yaz.
HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, false, procID);

struct view_matrix_t {
	float* operator[ ](int index) {
		return matrix[index];
	}
	float matrix[4][4];
};

struct Vector3 {
	float x, y, z;
};

Vector3 WorldToScreen(const Vector3 pos, view_matrix_t matrix) {
	float _x = matrix[0][0] * pos.x + matrix[0][1] * pos.y + matrix[0][2] * pos.z + matrix[0][3];
	float _y = matrix[1][0] * pos.x + matrix[1][1] * pos.y + matrix[1][2] * pos.z + matrix[1][3];

	float w = matrix[3][0] * pos.x + matrix[3][1] * pos.y + matrix[3][2] * pos.z + matrix[3][3];

	float inv_w = 1.f / w;
	_x *= inv_w;
	_y *= inv_w;

	float x = width * .5f;
	float y = height * .5f;

	x += 0.5f * _x * width + 0.5f;
	y -= 0.5f * _y * height + 0.5f;

	return{ x,y,w };
}

void DrawGoofyBox(HDC hdc, float sX, float sY, float eX, float eY)
{
	Rectangle(hdc, sX, sY, eX, eY);
}

void DrawLine(HDC hdc, float sX, float sY, float eX, float eY)
{
	int a, b = 0;
	HPEN hOPen;
	HPEN hNPen = CreatePen(PS_SOLID, 2, 0x000000EF);
	hOPen = (HPEN)SelectObject(hdc, hNPen);
	MoveToEx(hdc, sX, sY, NULL);
	a = LineTo(hdc, eX, eY);
	DeleteObject(SelectObject(hdc, hOPen));
}
void draw(HDC hdc, float xPos, float yPos, float zPos) {

	view_matrix_t matrix;
	for (int i = 0; i < 4; i++) {
		if (i != 2)
		{
			ReadProcessMemory(handle, LPCVOID(0x0057DFD0 + i * 16), &save, sizeof(save), nullptr);
			matrix[0][i] = save;
		}
		else
		{
			matrix[0][2] = 0;
		}
	}

	for (int i = 0; i < 4; i++) {
		ReadProcessMemory(handle, LPCVOID(0x0057DFD0 + 4 + 16 * i), &save, sizeof(save), nullptr);
		matrix[1][i] = save;
	}

	for (int i = 0; i < 4; i++) {
		ReadProcessMemory(handle, LPCVOID(0x0057DFD0 + 8 + 16 * i), &save, sizeof(save), nullptr);
		matrix[3][i] = save;
	}

	Vector3 pos = Vector3{ xPos,yPos,zPos };
	Vector3 screenPos = WorldToScreen(pos, matrix);
	DrawLine(hdc, width / 2.f, height, screenPos.x, screenPos.y);


	//Vector3 head;
	//head.x = pos.x;
	//head.y = pos.y;
	//head.z = pos.z + 50.f;
	//Vector3 screenHead = WorldToScreen(head, vm);
	//DrawGoofyBox(hdc, screenHead.x - 25.f, screenHead.y, screenPos.x + 5.f, screenPos.y + 20.f);

}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {

	case WM_CREATE: {
		SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);
		break;
	}
	case WM_ERASEBKGND: {
		return TRUE;
	}

	case WM_PAINT: {
		ReadProcessMemory(handle, LPCVOID(BaseModule + 0x00191FCC), &dusmanlarinclass, sizeof(dusmanlarinclass), nullptr);

		ReadProcessMemory(handle, LPCVOID(dusmanlarinclass + 0), &targetclass, sizeof(targetclass), nullptr);

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		SetBkMode(hdc, TRANSPARENT);
		HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
		FillRect(hdc, &ps.rcPaint, brush);
		DeleteObject(brush);
		ReadProcessMemory(handle, LPCVOID(targetclass + 0x4), &x, sizeof(x), nullptr);
		ReadProcessMemory(handle, LPCVOID(targetclass + 0x8), &y, sizeof(y), nullptr);
		ReadProcessMemory(handle, LPCVOID(targetclass + 0xC), &z, sizeof(z), nullptr);
		draw(hdc, x, y, z);
		Sleep(10);
		EndPaint(hwnd, &ps);
		InvalidateRect(hwnd, 0, TRUE);
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
	}
	default: {
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
		   return 0;
	}
}

int __stdcall WinMain(HINSTANCE instance, HINSTANCE pInstance, LPSTR lpCmd, int cmdShow) {
	const char* CLASS_NAME = "window class";
	WNDCLASSEX wc = { };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = CLASS_NAME;
	wc.hInstance = instance;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClassEx(&wc);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME, "esp",
		WS_EX_TRANSPARENT | WS_EX_TOPMOST, CW_USEDEFAULT, CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		instance,
		NULL
	);

	if (hwnd == NULL) {
		return 1;
	}

	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow(hwnd, cmdShow);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}