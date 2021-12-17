/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h> // sprintf
#include <windows.h>
#include <commctrl.h>

#include "fileopen.h"
#include "resource.h"

#define MAX_LOADSTRING 100

#define CenterBoth         0 // center application window on screen
#define CenterHorizontally 1 // center application window horizontally
#define CenterVertically   2 // center application window vertically

// Global Variables
HINSTANCE hInst;                     // current instance
TCHAR szTitle[MAX_LOADSTRING];       // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name

// Loader thread handle
HANDLE hLoaderThread = 0;

BOOL TerminateThreadsSignal,
     isCAPloaded = FALSE;

// Window handles
HWND hWnd, hStatus;

// DC handles
HDC  hMainDC, hPulseDC, hHeaderDC;

// User settings
BOOL isDoubleWindowWidth,
     isDoubleWindowHeight,
     isHalfwavesChecked,
     isFirstHalfwaveDarkGreenChecked,
     isUse1hBufferChecked,
     isUse2hBufferChecked,
     isUse5hBufferChecked;

// Visual settings
unsigned __int32 MSperLine = 10;

// Bitmap metrics
__int32 PulseBitmapWidth,
        PulseBitmapHeight,
        NormalPulseBitmapWidth  =  525,
        NormalPulseBitmapHeight =  480;
        DoublePulseBitmapWidth  = 1025,
        DoublePulseBitmapHeight =  700,
        HeaderBitmapHeight      =   16;

// ScrollBar specific
HWND       hScroll;
BOOL       isScrollBarEnabled = FALSE;
__int32    iScrollDelta = 1, siMax = 0;
SCROLLINFO si;

// ProgressBar specific
HWND    hProgress;
__int32 iProgressSteps = 100;

// Forward declarations
ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void             UpdateWindowMetrics(HWND hWnd, __int8 WhichCenter);


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
	MSG msg;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TAPVIEW, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	if (!InitInstance (hInstance, nCmdShow))
		return FALSE;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style         = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc   = WndProc;
	wcex.cbClsExtra    = 0;
	wcex.cbWndExtra    = 0;
	wcex.hInstance     = hInstance;
	wcex.hIcon         = NULL;
	wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName  = MAKEINTRESOURCE(IDC_TAPVIEW);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm       = NULL;

	return RegisterClassEx(&wcex);
}


BOOL InitBaseGfx(HINSTANCE hInst, HWND hWnd)
{
	HBITMAP    hPulseBitmap, hHeaderBitmap;
	RECT       rStatus, rParent;
	HDC        hDC;
	__int32    iVThumb, iVWidth;

	hDC = GetDC(hWnd);
	hPulseDC = CreateCompatibleDC(hDC);

	if (!hPulseDC)
		return FALSE;

	hPulseBitmap = CreateCompatibleBitmap(hDC, DoublePulseBitmapWidth, DoublePulseBitmapHeight);

	if (!hPulseBitmap)
		return FALSE;

	SelectObject(hPulseDC, hPulseBitmap);

	hHeaderDC = CreateCompatibleDC(hDC);

	if (!hHeaderDC)
		return FALSE;

	hHeaderBitmap = CreateCompatibleBitmap(hDC, DoublePulseBitmapWidth, HeaderBitmapHeight);

	if (!hHeaderBitmap)
		return FALSE;

	SelectObject(hHeaderDC, hHeaderBitmap);

	hStatus = CreateWindowEx(
			0,
			"msctls_statusbar32",
			"Open a CAP image to view.",
			WS_CHILD | WS_VISIBLE,
			0, 0, 0, 0,
			hWnd,
			(HMENU) IDC_STATUSBAR,
			hInst,
			NULL);

	if (!hStatus)
		return FALSE;

	hProgress = CreateWindowEx(
			0,
			"msctls_progress32",
			NULL,
			WS_CHILD,
			0, 0, 100, 10,
			hWnd,
			(HMENU) IDC_PROGRESSBAR,
			hInst,
			NULL);

	if (!hProgress)
		return FALSE;

	SendDlgItemMessage(hWnd, IDC_PROGRESSBAR, PBM_SETRANGE32, 0, iProgressSteps);
	SendDlgItemMessage(hWnd, IDC_PROGRESSBAR, PBM_SETSTEP, 1, 0);

	GetClientRect(hStatus, &rStatus);
	GetClientRect(hWnd, &rParent);
	iVThumb = GetSystemMetrics(SM_CYVTHUMB);
	iVWidth = GetSystemMetrics(SM_CXVSCROLL);

	hScroll = CreateWindowEx(
		0L,                     // no extended styles
		"SCROLLBAR",            // scroll bar control class
		(LPSTR) NULL,           // window title bar text
		WS_CHILD | SBS_VERT | WS_VISIBLE | WS_DISABLED, // scroll bar styles
		rParent.right-iVWidth,  // horizontal position
		rParent.top,            // vertical position
		iVWidth,                // scroll bar width
		rParent.bottom-(rStatus.bottom-rStatus.top), // scroll bar height
		hWnd,                   // main window handle
		(HMENU) IDC_SCROLLBAR,  // no menu for scroll bar
		hInst,                  // instance owning this window
		(LPVOID) NULL           // not needed
	);

	if (!hScroll)
		return FALSE;

	// Init ScrollBar
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = 0;
	si.nPage = PulseBitmapHeight;
	si.nPos = 0;
	SetScrollInfo(hScroll, SB_CTL, &si, TRUE);

	return TRUE;
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance;

//	ReadConfig();
	isDoubleWindowWidth = FALSE; isDoubleWindowHeight = FALSE;
	isHalfwavesChecked = TRUE; isFirstHalfwaveDarkGreenChecked = TRUE;
	isUse1hBufferChecked = TRUE; isUse2hBufferChecked = FALSE; isUse5hBufferChecked = FALSE;

	PulseBitmapWidth  = (isDoubleWindowWidth  ? DoublePulseBitmapWidth  : NormalPulseBitmapWidth);
	PulseBitmapHeight = (isDoubleWindowHeight ? DoublePulseBitmapHeight : NormalPulseBitmapHeight);

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, 0, PulseBitmapWidth+22, PulseBitmapHeight+HeaderBitmapHeight+64, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	hMainDC = GetDC(hWnd);

	if (!InitBaseGfx(hInst, hWnd))
		return FALSE;

	UpdateWindowMetrics(hWnd, CenterBoth);

	CheckMenuItem(GetMenu(hWnd), IDM_DOUBLE_SCREEN_WIDTH, (isDoubleWindowWidth ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(GetMenu(hWnd), IDM_DOUBLE_SCREEN_HEIGHT, (isDoubleWindowHeight ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(GetMenu(hWnd), IDM_SHOW_HALFWAVES, (isHalfwavesChecked ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(GetMenu(hWnd), IDM_FIRST_HALFWAVE_IN_DARK_GREEN, (isFirstHalfwaveDarkGreenChecked ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(GetMenu(hWnd), IDM_USE_1H_BUFFER, (isUse1hBufferChecked ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(GetMenu(hWnd), IDM_USE_2H_BUFFER, (isUse2hBufferChecked ? MF_CHECKED : MF_UNCHECKED));
	CheckMenuItem(GetMenu(hWnd), IDM_USE_5H_BUFFER, (isUse5hBufferChecked ? MF_CHECKED : MF_UNCHECKED));

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


void UpdateWindowMetrics(HWND hWnd, __int8 WhichCenter)
{
	RECT    rParent, rStatus, rWindow;
	__int32 iWindowWidth, iWindowHeight;
	__int32 iScrollWidth, iStatusHeight;
	__int32 iScreenWidth, iScreenHeight,
	        iWindowX = 0, iWindowY = 0;

	// Evaluate menu settings
	PulseBitmapWidth  = (isDoubleWindowWidth  ? DoublePulseBitmapWidth  : NormalPulseBitmapWidth);
	PulseBitmapHeight = (isDoubleWindowHeight ? DoublePulseBitmapHeight : NormalPulseBitmapHeight);

	GetClientRect(hWnd, &rParent);
	GetWindowRect(hWnd, &rWindow);
	GetClientRect(hStatus, &rStatus);

	iScrollWidth = GetSystemMetrics(SM_CXVSCROLL);
	iStatusHeight = rStatus.bottom - rStatus.top + 1;

	iWindowWidth = (rWindow.right - rWindow.left + 1) - (rParent.right - rParent.left + 1 - (PulseBitmapWidth + iScrollWidth));
	iWindowHeight = (rWindow.bottom - rWindow.top + 1) - (rParent.bottom - rParent.top + 1 - (HeaderBitmapHeight + PulseBitmapHeight + iStatusHeight));

	// Make sure window is fully visible on screen.
	iScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	iScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	if (((WhichCenter == CenterBoth) || (WhichCenter == CenterHorizontally))
		&& (rWindow.left+iWindowWidth > iScreenWidth))
			iWindowX = max(0, (iScreenWidth - iWindowWidth)/2);
	else
		iWindowX = rWindow.left;
	if (((WhichCenter == CenterBoth) || (WhichCenter == CenterVertically))
		&& (rWindow.top+iWindowHeight > iScreenHeight))
			iWindowY = max(0, (iScreenHeight - iWindowHeight)/2);
	else
		iWindowY = rWindow.top;

	SetWindowPos(hWnd, HWND_TOP, iWindowX, iWindowY, iWindowWidth, iWindowHeight, SWP_NOOWNERZORDER);

	GetClientRect(hWnd, &rParent);
	SetWindowPos(hStatus, HWND_TOP, rParent.left, rParent.bottom-iStatusHeight, rParent.right, iStatusHeight, SWP_NOOWNERZORDER);

	GetClientRect(hStatus, &rStatus);
	iStatusHeight = rStatus.bottom - rStatus.top + 1;
	SetWindowPos(hScroll, HWND_TOP, rParent.right-rParent.left-iScrollWidth, rParent.top, iScrollWidth, rParent.bottom-rParent.top-iStatusHeight, SWP_NOOWNERZORDER);

	// Update pulse window
	UpdateBitmapMetrics();
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int         wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC         hdc;

	__int32 oldPos;

	iScrollDelta = 1;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_OPENCAPIMAGE:
			if (OpenFileDialog(hWnd))
			{
				TerminateThreadsSignal = TRUE;
				isCAPloaded = FALSE;
				if (hLoaderThread != NULL)
					if (WaitForSingleObjectEx(hLoaderThread, 2000, FALSE) != WAIT_OBJECT_0)
						if (!TerminateThread(hLoaderThread, 0))
							if (WaitForSingleObjectEx(hLoaderThread, 2000, FALSE) != WAIT_OBJECT_0)
							{
								MessageBox(0, "Cancel load failed. Please retry or restart.", "Tapview", MB_SYSTEMMODAL | MB_ICONERROR);
								break;
							}
				TerminateThreadsSignal = FALSE;
				CloseHandle(hLoaderThread);
				if (StartLoaderThread() == FALSE)
				{
					MessageBox(0, "Thread error. Please retry or restart.", "Tapview", MB_SYSTEMMODAL | MB_ICONERROR);
					break;
				}
				isCAPloaded = TRUE;
			}
			break;
		case IDM_DOUBLE_SCREEN_WIDTH:
			isDoubleWindowWidth = !isDoubleWindowWidth;
			CheckMenuItem(GetMenu(hWnd), IDM_DOUBLE_SCREEN_WIDTH, (isDoubleWindowWidth ? MF_CHECKED : MF_UNCHECKED));
			UpdateWindowMetrics(hWnd, CenterHorizontally);
			//SaveConfig();
			if (isCAPloaded)
			{
				GetScrollInfo(hScroll, SB_CTL, &si);
				RepaintPic(si.nPos);
			}
			UpdateWindow(hWnd);
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_DOUBLE_SCREEN_HEIGHT:
			isDoubleWindowHeight = !isDoubleWindowHeight;
			CheckMenuItem(GetMenu(hWnd), IDM_DOUBLE_SCREEN_HEIGHT, (isDoubleWindowHeight ? MF_CHECKED : MF_UNCHECKED));
			UpdateWindowMetrics(hWnd, CenterVertically);
			//SaveConfig();
			if (isCAPloaded)
			{
				GetScrollInfo(hScroll, SB_CTL, &si);
				RepaintPic(si.nPos);
			}
			UpdateWindow(hWnd);
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_SHOW_HALFWAVES:
			isHalfwavesChecked = !isHalfwavesChecked;
			CheckMenuItem(GetMenu(hWnd), IDM_SHOW_HALFWAVES, (isHalfwavesChecked ? MF_CHECKED : MF_UNCHECKED));
			//SaveConfig();
			if (isCAPloaded)
			{
				GetScrollInfo(hScroll, SB_CTL, &si);
				RepaintPic(si.nPos);
			}
			UpdateWindow(hWnd);
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_FIRST_HALFWAVE_IN_DARK_GREEN:
			isFirstHalfwaveDarkGreenChecked = !isFirstHalfwaveDarkGreenChecked;
			CheckMenuItem(GetMenu(hWnd), IDM_FIRST_HALFWAVE_IN_DARK_GREEN, (isFirstHalfwaveDarkGreenChecked ? MF_CHECKED : MF_UNCHECKED));
			//SaveConfig();
			if (isCAPloaded)
			{
				GetScrollInfo(hScroll, SB_CTL, &si);
				RepaintPic(si.nPos);
			}
			UpdateWindow(hWnd);
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case IDM_USE_1H_BUFFER:
			isUse1hBufferChecked = TRUE;
			isUse2hBufferChecked = FALSE;
			isUse5hBufferChecked = FALSE;
			CheckMenuItem(GetMenu(hWnd), IDM_USE_1H_BUFFER, MF_CHECKED);
			CheckMenuItem(GetMenu(hWnd), IDM_USE_2H_BUFFER, MF_UNCHECKED);
			CheckMenuItem(GetMenu(hWnd), IDM_USE_5H_BUFFER, MF_UNCHECKED);
			//SaveConfig();
			break;
		case IDM_USE_2H_BUFFER:
			isUse1hBufferChecked = FALSE;
			isUse2hBufferChecked = TRUE;
			isUse5hBufferChecked = FALSE;
			CheckMenuItem(GetMenu(hWnd), IDM_USE_1H_BUFFER, MF_UNCHECKED);
			CheckMenuItem(GetMenu(hWnd), IDM_USE_2H_BUFFER, MF_CHECKED);
			CheckMenuItem(GetMenu(hWnd), IDM_USE_5H_BUFFER, MF_UNCHECKED);
			//SaveConfig();
			break;
		case IDM_USE_5H_BUFFER:
			isUse1hBufferChecked = FALSE;
			isUse2hBufferChecked = FALSE;
			isUse5hBufferChecked = TRUE;
			CheckMenuItem(GetMenu(hWnd), IDM_USE_1H_BUFFER, MF_UNCHECKED);
			CheckMenuItem(GetMenu(hWnd), IDM_USE_2H_BUFFER, MF_UNCHECKED);
			CheckMenuItem(GetMenu(hWnd), IDM_USE_5H_BUFFER, MF_CHECKED);
			//SaveConfig();
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		BitBlt(ps.hdc,
			0, 0,
			PulseBitmapWidth,
			HeaderBitmapHeight,
			hHeaderDC,
			0, 0,
			SRCCOPY);
		BitBlt(ps.hdc,
			0, HeaderBitmapHeight,
			PulseBitmapWidth,
			PulseBitmapHeight,
			hPulseDC,
			0, 0,
			SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_MOUSEWHEEL:
		if (!isScrollBarEnabled)
			return DefWindowProc(hWnd, message, wParam, lParam);
		wParam = MAKEWPARAM( ((short)HIWORD(wParam)<0) ? SB_LINEDOWN : SB_LINEUP, HIWORD(wParam));
		iScrollDelta = 1000/MSperLine;
	case WM_VSCROLL:
		si.fMask = SIF_ALL;
		GetScrollInfo(hScroll, SB_CTL, &si);
		oldPos = si.nPos;

		switch (LOWORD(wParam))
		{
		case SB_TOP:
			si.nPos = si.nMin;
			break;
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;
		case SB_LINEUP:
			si.nPos -= iScrollDelta;
			break;
		case SB_ENDSCROLL:
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;
		case SB_LINEDOWN:
			si.nPos += iScrollDelta;
			break;
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			break;
		case SB_BOTTOM:
			si.nPos = siMax;
			break;
		}

		if (si.nPos < si.nMin) si.nPos = si.nMin;
		if (si.nPos > (siMax-PulseBitmapHeight+1)) si.nPos = siMax-PulseBitmapHeight+1;
		if (si.nPos != oldPos)
		{
			si.fMask = SIF_POS | SIF_RANGE;
			si.nMax = siMax; // fix sync problem
			SetScrollInfo(hScroll, SB_CTL, &si, TRUE);
			RepaintPic(si.nPos);
			UpdateWindow(hWnd);
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
