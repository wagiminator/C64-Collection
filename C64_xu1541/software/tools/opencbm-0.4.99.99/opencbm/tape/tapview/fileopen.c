/*
 *  CBM 1530/1531 tape routines.
 *  Copyright 2012 Arnd Menge, arnd(at)jonnz(dot)de
*/

#include <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <malloc.h>

#include "resource.h"
#include "fileopen.h"
#include "cap.h"

#define Tape_Status_OK 1 // from tape.h

typedef struct sLineInfo {
	unsigned __int8  Info; // [1bit Empty/Data | 1bit FirstHW]
	unsigned __int32 SigOfs;
} *sLineInfo;

sLineInfo LineInfo;

// Global Variables
extern BOOL   TerminateThreadsSignal;
extern HANDLE hLoaderThread;

// Window handles
extern HWND hWnd, hStatus;

// DC handles
extern HDC hMainDC, hPulseDC, hHeaderDC;
HDC        hBaseDC, hBlackDC, hBlackHeaderDC;

// File specific
HANDLE           hCAP = 0;
char             szFile[1024];
unsigned __int16 *CAPbuf = NULL;

// User settings
extern BOOL isDoubleWindowWidth,
            isDoubleWindowHeight,
            isHalfwavesChecked,
            isFirstHalfwaveDarkGreenChecked,
            isUse1hBufferChecked,
            isUse2hBufferChecked,
            isUse5hBufferChecked;

// Visual settings
extern unsigned __int32 MSperLine;
unsigned __int32        HorizontalDivFact;

// Bitmap metrics
extern __int32 PulseBitmapWidth,
               PulseBitmapHeight,
               NormalPulseBitmapWidth,
               NormalPulseBitmapHeight,
               DoublePulseBitmapWidth,
               DoublePulseBitmapHeight,
               HeaderBitmapHeight;

// ScrollBar specific
extern HWND    hScroll;
extern BOOL    isScrollBarEnabled;
extern __int32 iScrollDelta, siMax;
SCROLLINFO     si2;

// ProgressBar specific
extern HWND    hProgress;
extern __int32 iProgressSteps;

// Forward declarations
void PaintVerticalRaster(HDC hRasterDC, HDC hHeaderDC);
void CopyDC(HDC hSourceDC, HDC hTargetDC);
void PaintRaster(int ScrollPos);


void UpdateBitmapMetrics(void)
{
	HBITMAP hBaseBitmap, hBlackBitmap, hBlackHeaderBitmap;

	ReleaseDC(hWnd, hBaseDC);
	ReleaseDC(hWnd, hBlackDC);
	ReleaseDC(hWnd, hBlackHeaderDC);

	hBaseDC = CreateCompatibleDC(hMainDC);
	hBaseBitmap = CreateCompatibleBitmap(hMainDC, PulseBitmapWidth, PulseBitmapHeight);
	SelectObject(hBaseDC, hBaseBitmap);

	hBlackDC = CreateCompatibleDC(hMainDC);
	hBlackBitmap = CreateCompatibleBitmap(hMainDC, PulseBitmapWidth, PulseBitmapHeight);
	SelectObject(hBlackDC, hBlackBitmap);

	hBlackHeaderDC = CreateCompatibleDC(hMainDC);
	hBlackHeaderBitmap = CreateCompatibleBitmap(hMainDC, DoublePulseBitmapWidth, HeaderBitmapHeight);
	SelectObject(hBlackHeaderDC, hBlackHeaderBitmap);

	CopyDC(hBlackHeaderDC, hHeaderDC);
	PaintVerticalRaster(hBlackDC, hHeaderDC);
	CopyDC(hBlackDC, hBaseDC);
	CopyDC(hBaseDC, hPulseDC);
	
	HorizontalDivFact = ((PulseBitmapWidth==DoublePulseBitmapWidth) ? 1 : 2);
}


void CopyDC(HDC hSourceDC, HDC hTargetDC)
{
	BitBlt(hTargetDC, 
		0,
		0,
		PulseBitmapWidth,
		PulseBitmapHeight,
		hSourceDC, 
		0,
		0,
		SRCCOPY); 

		UpdateWindow(hWnd);
		InvalidateRect(hWnd, NULL, FALSE);
}


// Paint vertical reference lines in red color (into hRasterDC).
// Paint time values in microseconds above reference lines (into hHeaderDC).
// Spacings depend on chosen window size (PulseBitmapHeight).
void PaintVerticalRaster(HDC hRasterDC, HDC hHeaderDC)
{
	__int32 i, j;
	__int32 iXDelta = (PulseBitmapWidth-PulseBitmapWidth%100)/10/2;
	char    timecode[6];

	SetTextColor(hHeaderDC, 0x00FFFFFF);
	SetBkMode(hHeaderDC, TRANSPARENT);

	for (i=0;i<10;i++)
	{
		for (j=0;j<PulseBitmapHeight;j++)
		{
			if (j % 3 == 0)
				SetPixel(hRasterDC, iXDelta+i*2*iXDelta, j, 0x0000009F);
			SetPixel(hRasterDC, 2*iXDelta*(i+1), j, 0x000000FF);
		}
		if (i < 9)
			sprintf(timecode, "%d00us", i+1);
		else
			sprintf(timecode, "1ms   ");
		TextOut(hHeaderDC, 2*(i+1)*iXDelta-((i < 9) ? 20:13), 0, timecode, ((i < 9) ? 5 : 3) );
	}
}


// CAP file loading thread.
// Loads chosen CAP file data into memory structure and indexes it for fast lookup (user navigation through mouse and scrollbar).
// Updates scrollbar while loading so user can immediately start scrolling pulse visualization.
DWORD WINAPI LoaderThreadFunction(LPVOID lpParam)
{
	__int32          i, iFileSize, NumLines, Counter = 0, NewPos, OldPos = 0, MAX_RANGE = 0;
	unsigned __int64 ui64Delta, ui64Len, ui64Abs, ui64Rel;
	unsigned __int32 ui32Len, ui32Line = 0;
	unsigned __int32 Timer_Precision_MHz;
	unsigned __int32 SigCount = 0;
	BOOL             FirstHW = FALSE, DCCopy = FALSE;

	// Cleanup
	if (hCAP) CAP_CloseFile(&hCAP);

	// Open CAP file and inspect.
	CAP_OpenFile(&hCAP, szFile);
	CAP_GetFileSize(hCAP, &iFileSize);
	CAP_ReadHeader(hCAP);
	if (CAP_isValidHeader(hCAP) != CAP_Status_OK)
	{
		MessageBox(0, "Invalid CAP file header.", "Tapview", MB_SYSTEMMODAL | MB_ICONERROR);
		return 0;
	}

	// Inform user about opened CAP file.
	SetWindowText(hStatus, szFile);

	// Cleanup
	if (CAPbuf) free(CAPbuf);
	if (LineInfo) free(LineInfo);

	// Get CAP file precision.
	CAP_GetHeader_Precision(hCAP, &Timer_Precision_MHz);

	// Reset DC, ProgressBar, ScrollBar
	CopyDC(hBlackDC, hBaseDC);
	SendDlgItemMessage(hWnd, IDC_PROGRESSBAR, PBM_SETPOS, 0, 0);
	SetWindowPos(hProgress, HWND_TOP, PulseBitmapWidth-100, 0, 0, 0, SWP_NOSIZE);
	SetForegroundWindow(hProgress);
	ShowWindow(hProgress, SW_SHOW);
	si2.cbSize = sizeof(SCROLLINFO);
	si2.fMask = SIF_POS | SIF_RANGE;
	si2.nMin = 0;
	si2.nMax = 0;
	siMax = 0;
	si2.nPage = PulseBitmapHeight;
	si2.nPos = 0;
	SetScrollInfo(hScroll, SB_CTL, &si2, TRUE);
	isScrollBarEnabled = TRUE;
	EnableScrollBar(hScroll, SB_CTL, ESB_ENABLE_BOTH);

	// Allocate data buffer for indexing CAP file data.
	// Each visualized line on screen corresponds to specific timespan (10ms by default).
	// As we don't know the total time of the CAP file before we finished loading,
	// we need to rely on the user menu setting.
	if (isUse1hBufferChecked)
		NumLines = 360000; // 360000 lines @ 10ms = 1h, x5 = 1.8MB
	else if (isUse2hBufferChecked)
		NumLines = 2*360000;
	else
		NumLines = 5*360000;
	LineInfo = malloc(sizeof(struct sLineInfo)*NumLines);
	if (LineInfo == NULL)
	{
		MessageBox(0, "Memory allocation failed (#1).", "Tapview", MB_SYSTEMMODAL | MB_ICONERROR);
		return FALSE;
	}
	for (i=0;i<NumLines;i++)
		LineInfo[i].Info = 0;

	// Allocate data buffer for CAP file data.
	// Only pulse lenghts between ~25us and 1000us are to be shown,
	// hence 2 bytes per pulse are sufficient (instead of 5 bytes).
	CAPbuf = (unsigned __int16 *) malloc(iFileSize/5*2); // #Signals x 2byte
	if (CAPbuf == NULL)
	{
		MessageBox(0, "Memory allocation failed (#2).", "Tapview", MB_SYSTEMMODAL | MB_ICONERROR);
		return FALSE;
	}

	// Prepare pulse bitmap immediately shown to user while loading.
	// Paint horizontal reference lines in red color (into hBaseDC).
	// Paint time codes just below reference lines (into hBaseDC).
	// Spacings depend on chosen window size (PulseBitmapWidth).
	PaintRaster(0);

	// Read initial pause (until first falling edge).
	if (CAP_ReadSignal(hCAP, &ui64Abs, &Counter) != CAP_Status_OK)
		return 0;

	// Load CAP file data into memory structure and index it for fast lookup (user navigation through mouse and scrollbar).
	// Update scrollbar while loading so user can immediately start scrolling pulse visualization.
	while (!TerminateThreadsSignal)
	{
		// Update flag to indicate if pulse length corresponds to first or second half wave.
		// First half waves are visualized in dark green by default.
		FirstHW = !FirstHW;

		// Read pulse.
		if (CAP_ReadSignal(hCAP, &ui64Delta, &Counter) != CAP_Status_OK)
			break;

		// Keep track of absolute tape time (from start of tape).
		ui64Abs += ui64Delta;

		// Relative pulse length in microseconds.
		ui64Rel = ui64Delta/Timer_Precision_MHz;

		// Only pulse lenghts <1000us are to be shown.
		if (ui64Rel < 1000)
		{
			// Absolute pulse time in microseconds (from start of tape) defines on which line to paint pulse.
			ui64Len = ui64Abs/Timer_Precision_MHz;

			// Calculate line on which to paint pulse.
			// Each line corresponds to specific timespan (10ms by default).
			ui32Line = (unsigned __int32) ui64Len/1000/MSperLine;

			ui32Len = (unsigned __int32) ui64Rel;

			// Prepare immediate visual for user, set flag and copy to hPulseDC if ready to show to user.
			if (abs(ui32Line) <= PulseBitmapHeight)
				SetPixel(hBaseDC, ui32Len/HorizontalDivFact, ui32Line, (FirstHW & isFirstHalfwaveDarkGreenChecked) ? 0x00007F00 : 0x0000FF00 );
			else if (!DCCopy)
			{
				DCCopy = TRUE;
				CopyDC(hBaseDC, hPulseDC);
			}

			// Store signal to buffer for later lookup (user navigation through mouse and scrollbar).
			CAPbuf[SigCount] = (unsigned __int16) ui32Len;

			// Check if index buffer is still large enough.
			if (abs(ui32Line) >= NumLines)
			{
				MessageBox(0, "Buffer full.\nPlease select larger buffer in menu and reload.", "Tapview", MB_SYSTEMMODAL | MB_ICONERROR);
				break;
			}

			// Update index buffer for fast pulse buffer lookup.
			// Each index points to a pulse in the pulse buffer (.SigOfs).
			// Each index corresponds to a visualized line on screen which itself corresponds to a specific timespan (10ms by default).
			// All pulses between two indexes are painted onto the same line on screen,
			// hence only first pulse on each line is indexed.
			// If corresponding visualized line is empty .Info MSB is zero.
			// Second highest .Info bit flags if corresponding pulse is first or second half wave.
			if (LineInfo[ui32Line].Info == 0)
			{
				LineInfo[ui32Line].Info = 0x80 | (FirstHW ? 0x40 : 0);
				LineInfo[ui32Line].SigOfs = SigCount;
			}

			// Keep track of number of signals.
			SigCount++;
		}

		// Update ProgressBar and ScrollBar if necessary.
		// iFileSize is divided into iProgressSteps steps (100 steps by default).
		// Update if signal counter passes a step.
		NewPos = Counter/(iFileSize/iProgressSteps);
		if (NewPos > OldPos)
		{
			// Update ProgressBar
			SendDlgItemMessage(hWnd, IDC_PROGRESSBAR, PBM_STEPIT, 0, 0);
			UpdateWindow(hProgress);
			OldPos = NewPos;

			// Update ScrollBar
			si2.fMask = SIF_RANGE | SIF_PAGE;
			siMax = max(0, (__int32)(((__int32) 0) + ui32Line));
			si2.nMax = siMax;
			SetScrollInfo(hScroll, SB_CTL, &si2, TRUE);
		}
		//for (i=0;i<1;i++)
		//	OutputDebugString(TEXT("Wait"));
	}

	// Close CAP file.
	CAP_CloseFile(&hCAP);

	// Stop immediately if we got the terminate flag.
	if (TerminateThreadsSignal)
		return 0;

	// Make sure picture is shown to user.
	if (!DCCopy)
		CopyDC(hBaseDC, hPulseDC);

	// Final ScrollBar update.
	MAX_RANGE = (__int32) (((__int32) 0) + (unsigned __int32) (ui64Abs/Timer_Precision_MHz/1000/MSperLine));
	si2.fMask = SIF_RANGE | SIF_PAGE;
	siMax = max(0, MAX_RANGE);
	si2.nMax = siMax;
	SetScrollInfo(hScroll, SB_CTL, &si2, TRUE);

	// Hide ProgressBar and repaint window.
	ShowWindow(hProgress, SW_HIDE);
	InvalidateRect(hWnd, NULL, FALSE);

	return 0; // 0 = thread exit ok
}


// Paint pulses to hBaseDC (pulse picture is double buffered), starting at specified index buffer index (ScrollPos).
// Each index points to a pulse in the pulse buffer (.SigOfs).
// Each index corresponds to a visualized line on screen which itself corresponds to a specific timespan (10ms by default).
// All pulses between two indexes are painted onto the same line on screen,
// hence only first pulse on each line is indexed.
// If corresponding visualized line is empty .Info MSB is zero.
// Second highest .Info bit flags if corresponding pulse is first or second halfwave.
// By default first halfwave is painted in dark green, second in light green.
void PaintPulses(int ScrollPos)
{
	unsigned __int16 Pulse;
	__int32          Line, i, SigStart, SigEnd;
	BOOL             FirstHW;

	// Loop through all pulse picture lines.
	for (Line=0;Line<PulseBitmapHeight;Line++)
	{
		// Line has pulses if .Info MSB is one.
		if (LineInfo[ScrollPos+Line].Info & 0x80)
		{
			// All pulses between two indexes are painted onto the same line on screen.
			SigStart = LineInfo[ScrollPos+Line  ].SigOfs;
			SigEnd   = LineInfo[ScrollPos+Line+1].SigOfs;

			// Second highest .Info bit flags if corresponding pulse is first or second halfwave.
			FirstHW = (LineInfo[ScrollPos+Line].Info & 0x40) ? TRUE : FALSE;

			// Get first half wave if ScrollPos happens to point to a second halfwave.
			if (!FirstHW)
				if (!isHalfwavesChecked)
					if (SigStart > 0)
						SigStart--;

			// Paint all pulses between two indexes onto the same line on screen.
			i = SigStart;
			while (i < SigEnd-1)
			{
				// Get length of first halfwave.
				Pulse = CAPbuf[i];

				// Add corresponding second halfwave if user wants to view only full waves.
				if (!isHalfwavesChecked)
					Pulse+= CAPbuf[++i];

				// Only pulse lengths <1000us are painted.
				// HorizontalDivFact is 2 for normal window width, and 1 for double/enlarged window width.
				// By default first halfwave is painted in dark green, second in light green.
				if (Pulse < 1000)
					SetPixel(hBaseDC, Pulse/HorizontalDivFact, Line, (FirstHW && isFirstHalfwaveDarkGreenChecked && isHalfwavesChecked) ? 0x00007F00 : 0x0000FF00 );
				i++;

				// Keep track if next halfwave is first or second,
				// only relevant when user wants to view halfwaves (isHalfwavesChecked==TRUE).
				FirstHW = !FirstHW;
			}
		}
	}
}


// Creates and starts the CAP file loading thread.
// CAP file loading thread
// - loads chosen CAP file data into memory structure and indexes it for fast lookup (user navigation through mouse and scrollbar).
// - updates scrollbar while loading so user can immediately start scrolling pulse visualization.
BOOL StartLoaderThread(void)
{
	DWORD dwLoaderThreadId;

	hLoaderThread = CreateThread(NULL, 0, LoaderThreadFunction, NULL, 0, &dwLoaderThreadId);

	if (hLoaderThread == NULL)
	{
		MessageBox(0, "Creating thread failed. Please restart.", "Tapview", MB_SYSTEMMODAL | MB_ICONERROR);
		return FALSE;
	}

	return TRUE;
}


// Paint horizontal reference lines in red color (into hBaseDC).
// Paint time codes just below reference lines (into hBaseDC).
// Spacings depend on chosen window size (PulseBitmapWidth).
void PaintRaster(int ScrollPos)
{
	__int32 Line, j;
	__int32 hrs = 0, mins = 0, secs = 0;
	char    timecode[12];
	__int32 numsecs;
	RECT    rt = {PulseBitmapWidth-70, 0, PulseBitmapWidth, 0};

	numsecs = ScrollPos*MSperLine/1000;
	secs = numsecs % 60;
	hrs  = numsecs/3600;
	mins = (numsecs - hrs*3600)/60;

	Line = (1000/MSperLine-1) - (ScrollPos % (1000/MSperLine));

	while (Line < PulseBitmapHeight)
	{
		for (j=0;j<PulseBitmapWidth;j++)
			SetPixel(hBaseDC, j, Line, 0x000000FF);
		secs++;
		if (secs == 60)
		{
			mins++;
			secs = 0;
			if (mins == 60)
			{
				hrs++;
				mins = 0;
			}
		}
		sprintf(timecode, "%.2u:%.2u:%.2u", hrs, mins, secs);
		rt.top = Line+1;
		rt.bottom = Line+1+16;
		SetTextColor(hBaseDC, 0x00FFFFFF);
		SetBkMode(hBaseDC, TRANSPARENT);
		DrawText(hBaseDC, timecode, 8, &rt, DT_TOP | DT_RIGHT | DT_SINGLELINE);
		Line += 1000/MSperLine;
	}
}


void RepaintPic(int ScrollPos)
{
	CopyDC(hBlackDC, hBaseDC);
	PaintRaster(ScrollPos);
	PaintPulses(ScrollPos);
	CopyDC(hBaseDC, hPulseDC);
}


BOOL OpenFileDialog(HWND hWnd)
{
	OPENFILENAME *ofn;
	DWORD ofn_size;
	char  szPath[1024] = "\0";
    BOOL  hr;

	OSVERSIONINFO osi = {0};
	osi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osi);
	if ((osi.dwMajorVersion >=5) && (osi.dwPlatformId == VER_PLATFORM_WIN32_NT))
		ofn_size = sizeof(OPENFILENAME);
	else
		ofn_size = OPENFILENAME_SIZE_VERSION_400;

	ofn = (OPENFILENAME*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ofn_size);
	if (ofn == NULL)
	{
		MessageBox(hWnd,TEXT("HeapAlloc error"),TEXT("HeapAlloc error"),MB_ICONWARNING|MB_TOPMOST);
		return FALSE;
	}

	sprintf(szFile, "*.CAP\0");

	ofn->lStructSize       = ofn_size;
	ofn->hwndOwner         = hWnd;
	ofn->hInstance         = GetModuleHandle(NULL);
	ofn->lpstrFilter       = "ZoomTape CAP Image (*.CAP)\0*.CAP\0\0";
	ofn->lpstrCustomFilter = NULL;
	ofn->nMaxCustFilter    = 0;
	ofn->nFilterIndex      = 1;
	ofn->lpstrFile         = szFile;
	ofn->nMaxFile          = ARRAYSIZE(szFile);
	ofn->lpstrFileTitle    = NULL;
	ofn->nMaxFileTitle     = 0;
	ofn->lpstrInitialDir   = szPath;
	ofn->lpstrTitle        = "Load CAP image";
	ofn->Flags             = (OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_ENABLESIZING|OFN_HIDEREADONLY);
	ofn->nFileOffset       = 0;
	ofn->nFileExtension    = 0;
	ofn->lpstrDefExt       = "CAP";
	ofn->lCustData         = (LPARAM) NULL;
	ofn->lpfnHook          = NULL;
	ofn->lpTemplateName    = NULL;
	if ((osi.dwMajorVersion >=5) && (osi.dwPlatformId == VER_PLATFORM_WIN32_NT))
	{
		ofn->pvReserved    = NULL;
		ofn->dwReserved    = 0;
		ofn->FlagsEx       = 0;
	}

	hr = GetOpenFileName(ofn);

	if (HeapFree(GetProcessHeap(), 0, ofn) == 0)
	{
		MessageBox(hWnd, TEXT("HeapFree error"), TEXT("HeapFree error"), MB_ICONWARNING|MB_TOPMOST);
		return FALSE;
	}

	return hr;
}
