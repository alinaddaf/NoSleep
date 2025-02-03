
// mouseMoverDlg.cpp : implementation file

#include "pch.h"
#include "framework.h"
#include "mouseMover.h"
#include "mouseMoverDlg.h"
#include "afxdialogex.h"
#include <windows.h>
#include <stdio.h>
//#include "stdafx.h"
#include "pch.h"  
#include <AfxWin.h>

#pragma warning(disable : 4996)
#ifdef _DEBUG
#define __DEBUG_MODE__  
#endif
#define VERSION 1.1
#define MOUSE_MOVE_FREQUENCY  3*60*1000	// In Milliseconds (3 minutes)
#define HORIZONTAL_MOUSE_MOVE 2         // Number of horizontal pixels to jump to
#define VERTICAL_MOUSE_MOVE   0			// Number of Vertical pixeld to jump to
#define MOUSE_MOVE_PAUSE     100 		// In Milliseconds
#define TIMER_ID 1  // timer ID
FILE* ofile = NULL;
char fileName[128] = "mouseMoverLog.txt";
char printBuf[100];
char currentTime[100];
UINT_PTR g_TimerID = 0;
bool startTimer = false;
CStatic* m_static1;

CButton* enableNoSleep, * disableNoSleep;
POINT currentMousePosition, oldMousePosition;
INPUT mouseInput = { 0 };

void PrintWithTimestamp(const char* message) {
#ifdef __DEBUG_MODE__
	char buf[100]; // Local buffer to ensure thread safety

	if (!message) {
		message = "(null)"; // Handle NULL input
	}

	SYSTEMTIME time;
	GetSystemTime(&time); // Retrieve the current system time (UTC)

	// Format the message with timestamp safely
	snprintf(buf, sizeof(buf), "[%02d:%02d:%02d.%03d] %s\n",
		time.wHour,
		time.wMinute,
		time.wSecond,
		time.wMilliseconds,
		message);
	fputs(buf, ofile);
#endif
}
DWORD GetLastMouseMovementTime()
{
	LASTINPUTINFO li;
	li.cbSize = sizeof(LASTINPUTINFO);

	if (!GetLastInputInfo(&li))
	{
		//m_static1->SetWindowTextW(CString("Failed to retrieve last input info"));
		//printf("Failed to retrieve last input info. Error: %lu\n", GetLastError());
		return 0;
	}
	return li.dwTime;
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CmouseMoverDlg dialog

CmouseMoverDlg::CmouseMoverDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MOUSEMOVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CmouseMoverDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CmouseMoverDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_EXIT, &CmouseMoverDlg::OnBnClickedExit)
	ON_BN_CLICKED(IDC_ENABLE_NO_SLEEP, &CmouseMoverDlg::OnBnClickedEnableNoSleep)
	ON_BN_CLICKED(IDC_DISABLE_NO_SLEEP, &CmouseMoverDlg::OnBnClickedDisableNoSleep)
END_MESSAGE_MAP()


// CmouseMoverDlg message handlers

BOOL CmouseMoverDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// check to make sure another isnstance of the program is not running
	CreateMutexA(0, FALSE, "mouseMover"); // try to create a named mutex
	if (GetLastError() == ERROR_ALREADY_EXISTS) {// did the mutex already exist?
		AfxMessageBox(_T("Can't run because another instance of the program is running\nPlease kill the other program and try again"));
		CDialogEx::OnCancel();
		return -1; // quit; mutex is released automatically
	}
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	CStatic* m_static1 = (CStatic*)GetDlgItem(IDC_STATIC); //(tag)
	enableNoSleep = (CButton*)GetDlgItem(IDC_ENABLE_NO_SLEEP);
	disableNoSleep = (CButton*)GetDlgItem(IDC_DISABLE_NO_SLEEP);
	enableNoSleep->EnableWindow(TRUE);
	disableNoSleep->EnableWindow(FALSE);
	oldMousePosition.x = 0;
	oldMousePosition.y = 0;
	// Initialize the INPUT structure for mouse movement
	mouseInput.type = INPUT_MOUSE; // Specify mouse input

#ifdef __DEBUG_MODE__
	if (ofile == NULL) {
		ofile = fopen(fileName, "w");
		PrintWithTimestamp("Program Started");
	}
#endif
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CmouseMoverDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CmouseMoverDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}
 
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CmouseMoverDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

ULONGLONG GetTimeSinceLastInput() {
	LASTINPUTINFO lii = { 0 };
	lii.cbSize = sizeof(LASTINPUTINFO);

	if (GetLastInputInfo(&lii)) {
		return GetTickCount64() - lii.dwTime;
	}
	return 0; // Error case
}
void JiggleMouse(void) {
	oldMousePosition.x = currentMousePosition.x;
	oldMousePosition.y = currentMousePosition.y;
	// Move the mouse by some specified pixels in both X and Y directions
	mouseInput.mi.dx = HORIZONTAL_MOUSE_MOVE;
	mouseInput.mi.dy = VERTICAL_MOUSE_MOVE;
	mouseInput.mi.dwFlags = MOUSEEVENTF_MOVE; // Indicate relative movement
	SendInput(1, &mouseInput, sizeof(INPUT));
	Sleep(MOUSE_MOVE_PAUSE); // 500 ms
	// Restore the original mouse position
	mouseInput.mi.dx = -HORIZONTAL_MOUSE_MOVE;
	mouseInput.mi.dy = -VERTICAL_MOUSE_MOVE;
	mouseInput.mi.dwFlags = MOUSEEVENTF_MOVE; // Indicate relative movement
	SendInput(1, &mouseInput, sizeof(INPUT));
}
// Function to handle the timer event
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	GetCursorPos(&currentMousePosition);
	ULONGLONG timeElapsed = GetTimeSinceLastInput();
	if ((currentMousePosition.x == oldMousePosition.x) || (currentMousePosition.y == oldMousePosition.y)) {
		JiggleMouse();
#ifdef __DEBUG_MODE__
		sprintf(printBuf, "Mouse Jiggled after no movement, timeElapsed=%llu, x=%d, y=%d", timeElapsed, currentMousePosition.x, currentMousePosition.y);
		PrintWithTimestamp(printBuf);
#endif
	}
	else {
		if (timeElapsed > MOUSE_MOVE_FREQUENCY) {
			JiggleMouse();
#ifdef __DEBUG_MODE__
			sprintf(printBuf, "Mouse Jiggled after mouse movement, timeElapesd=%llu, x=%d, y=%d", timeElapsed, currentMousePosition.x, currentMousePosition.y);
			PrintWithTimestamp(printBuf);
#endif
		}
		else {
			oldMousePosition.x = currentMousePosition.x;
			oldMousePosition.y = currentMousePosition.y;
#ifdef __DEBUG_MODE__
			sprintf(printBuf, "Mouse jiggle bypassed, timeElapsed=%llu, x=%d, y=%d", timeElapsed, currentMousePosition.x, currentMousePosition.y);
			PrintWithTimestamp(printBuf);
#endif
		}
	}
	
}
int  MoveMouse()
{
	// Create a timer that fires every 5 minutes (in milliseconds)
	// SetTimer(NULL, TIMER_ID, 5 * 60 * 1000, &TimerProc);
	if (startTimer == true) {
		g_TimerID = SetTimer(NULL, TIMER_ID, 1 * MOUSE_MOVE_FREQUENCY, &TimerProc);
		PrintWithTimestamp("Timer Started");
	}
	else {
		// Stop the timer when the user presses Enter
		if (g_TimerID != 0) {
			KillTimer(NULL, g_TimerID);
			g_TimerID = 0; // Reset the timer ID
			PrintWithTimestamp("Timer Stopped");
		}
	}
	return 0;
}

void CmouseMoverDlg::OnBnClickedExit()
{
	PrintWithTimestamp("Program Terminated");
	if (ofile != NULL)
		fclose(ofile);
	CDialogEx::OnCancel();

	exit(0);
}

void CmouseMoverDlg::OnBnClickedEnableNoSleep()
{
	startTimer = true;
	enableNoSleep = (CButton*)GetDlgItem(IDC_ENABLE_NO_SLEEP);
	disableNoSleep = (CButton*)GetDlgItem(IDC_DISABLE_NO_SLEEP);
	enableNoSleep->EnableWindow(FALSE);
	disableNoSleep->EnableWindow(TRUE);
	MoveMouse();
}

void CmouseMoverDlg::OnBnClickedDisableNoSleep()
{
	startTimer = false;
	enableNoSleep = (CButton*)GetDlgItem(IDC_ENABLE_NO_SLEEP);
	disableNoSleep = (CButton*)GetDlgItem(IDC_DISABLE_NO_SLEEP);
	enableNoSleep->EnableWindow(TRUE);
	disableNoSleep->EnableWindow(FALSE);
	MoveMouse();
}
