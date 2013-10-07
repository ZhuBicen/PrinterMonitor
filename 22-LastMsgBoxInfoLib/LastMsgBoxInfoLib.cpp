/******************************************************************************
Module:  LastMsgBoxInfoLib.cpp
Notices: Copyright (c) 2008 Jeffrey Richter & Christophe Nasarre
******************************************************************************/


#include "..\CommonFiles\CmnHdr.h"
#include <WindowsX.h>
#include <tchar.h>
#include <stdio.h>
#include "APIHook.h"

#define LASTMSGBOXINFOLIBAPI extern "C" __declspec(dllexport)
#include "LastMsgBoxInfoLib.h"
#include <StrSafe.h>


///////////////////////////////////////////////////////////////////////////////


// Prototypes for the hooked functions
typedef int (WINAPI *PFNMESSAGEBOXA)(HWND hWnd, PCSTR pszText, 
   PCSTR pszCaption, UINT uType);

typedef int (WINAPI *PFNMESSAGEBOXW)(HWND hWnd, PCWSTR pszText, 
   PCWSTR pszCaption, UINT uType);

typedef int (WINAPI *PFNSTARTPAGE)(HDC hdc);

typedef int (WINAPI *PFNSTARTDOCA)(__in HDC hdc, __in CONST DOCINFOA *lpdi);
typedef int (WINAPI *PFNSTARTDOCW)(__in HDC hdc, __in CONST DOCINFOW *lpdi);

typedef BOOL (WINAPI *PFNOPENPRINTERA)(  _In_   LPSTR pPrinterName,  _Out_  LPHANDLE phPrinter,   _In_   LPPRINTER_DEFAULTSA pDefault);
typedef BOOL (WINAPI *PFNOPENPRINTERW)(  _In_   LPWSTR pPrinterName,  _Out_  LPHANDLE phPrinter,  _In_   LPPRINTER_DEFAULTSW pDefault);

typedef HDC  (WINAPI *PFNCREATEDCA)( __in_opt LPCSTR pwszDriver, __in_opt LPCSTR pwszDevice, __in_opt LPCSTR pszPort, __in_opt CONST DEVMODEA * pdm);
typedef HDC  (WINAPI *PFNCREATEDCW)( __in_opt LPCWSTR pwszDriver, __in_opt LPCWSTR pwszDevice, __in_opt LPCWSTR pszPort, __in_opt CONST DEVMODEW * pdm);
// We need to reference these variables before we create them.
//extern CAPIHook g_OpenPrinterA;
//extern CAPIHook g_OpenPrinterW;
//
extern CAPIHook g_CreateDCA;
extern CAPIHook g_CreateDCW;
//
extern CAPIHook g_StartPage;
extern CAPIHook g_StartDocA;
extern CAPIHook g_StartDocW;

///////////////////////////////////////////////////////////////////////////////


// This function sends the MessageBox info to our main dialog box
void SendLastMsgBoxInfo(BOOL bUnicode, PVOID docName, int pageNum) {

   // Get the pathname of the process displaying the message box
   wchar_t szProcessPathname[MAX_PATH];
   GetModuleFileNameW(NULL, szProcessPathname, MAX_PATH);

   // Construct the string to send to the main dialog box
   wchar_t sz[2048];
   StringCchPrintfW(sz, _countof(sz), bUnicode 
      ? L"Process: (%d) %s\r\nDoc: %s\r\nPage: %d\r\n"
      : L"Process: (%d) %s\r\nDoc: %S\r\nPage: %d\r\n",
      GetCurrentProcessId(), szProcessPathname,
      docName, pageNum);

   // Send the string to the main dialog box
   COPYDATASTRUCT cds = { 0, ((DWORD)wcslen(sz) + 1) * sizeof(wchar_t), sz };
   FORWARD_WM_COPYDATA(FindWindow(NULL, TEXT("Last MessageBox Info")), 
      NULL, &cds, SendMessage);
}
void SendLastMsgBoxInfo(BOOL bUnicode, PVOID printerName) {

   // Get the pathname of the process displaying the message box
   wchar_t szProcessPathname[MAX_PATH];
   GetModuleFileNameW(NULL, szProcessPathname, MAX_PATH);

   // Construct the string to send to the main dialog box
   wchar_t sz[2048];
   StringCchPrintfW(sz, _countof(sz), bUnicode 
      ? L"Process: (%d) %s\r\nPrinter: %s\r\n"
      : L"Process: (%d) %s\r\nPrinter: %S\r\n",
      GetCurrentProcessId(), szProcessPathname,
      printerName);

   // Send the string to the main dialog box
   COPYDATASTRUCT cds = { 0, ((DWORD)wcslen(sz) + 1) * sizeof(wchar_t), sz };
   FORWARD_WM_COPYDATA(FindWindow(NULL, TEXT("Last MessageBox Info")), 
      NULL, &cds, SendMessage);
}
int g_pageNum = 0;
BOOL g_unicode = FALSE;
///////////////////////////////////////////////////////////////////////////////
int WINAPI Hook_StartPage(__in HDC hdc) {
	int nResult = ((PFNSTARTPAGE)(PROC) g_StartPage)(hdc);
	SendLastMsgBoxInfo(FALSE, (PVOID)"Printing Page", g_pageNum++);
	return nResult;
}

int Hook_StartDocA(__in HDC hdc, __in CONST DOCINFOA *lpdi) {
	g_pageNum = 0;
	if (MessageBoxA(NULL, lpdi->lpszDocName, "print?", MB_YESNO) == IDYES)
	{
		SendLastMsgBoxInfo(FALSE, (PVOID)lpdi->lpszDocName, 0);
		return ((PFNSTARTDOCA)(PROC) g_StartDocA)(hdc, lpdi);
	}
}
int Hook_StartDocW(__in HDC hdc, __in CONST DOCINFOW *lpdi) {
	g_pageNum = 0;
	if (MessageBoxW(NULL, lpdi->lpszDocName, L"print?", MB_YESNO) == IDYES)
	{
		SendLastMsgBoxInfo(TRUE, (PVOID)lpdi->lpszDocName, 0);
		return ((PFNSTARTDOCW)(PROC) g_StartDocW)(hdc, lpdi);
	}
}

//int Hook_OpenPrinterA(  _In_   LPSTR pPrinterName,  _Out_  LPHANDLE phPrinter,   _In_   LPPRINTER_DEFAULTSA pDefault) {
//	return ((PFNOPENPRINTERA)(PROC) g_OpenPrinterA)(pPrinterName, phPrinter, pDefault);
//}
//
//int Hook_OpenPrinterW(_In_   LPWSTR pPrinterName,  _Out_  LPHANDLE phPrinter,  _In_   LPPRINTER_DEFAULTSW pDefault) {
//	return ((PFNOPENPRINTERW)(PROC) g_OpenPrinterW)(pPrinterName, phPrinter, pDefault);
//}
//
HDC     WINAPI Hook_CreateDCA( __in_opt LPCSTR pwszDriver, __in_opt LPCSTR pwszDevice, __in_opt LPCSTR pszPort, __in_opt CONST DEVMODEA * pdm){
	SendLastMsgBoxInfo(FALSE, (PVOID)pwszDevice);
	return ((PFNCREATEDCA)(PROC) g_CreateDCA)(pwszDriver, pwszDevice, pszPort, pdm);
}
HDC     WINAPI Hook_CreateDCW( __in_opt LPCWSTR pwszDriver, __in_opt LPCWSTR pwszDevice, __in_opt LPCWSTR pszPort, __in_opt CONST DEVMODEW * pdm){
	SendLastMsgBoxInfo(TRUE, (PVOID)pwszDevice);
	return ((PFNCREATEDCW)(PROC) g_CreateDCW)(pwszDriver, pwszDevice, pszPort, pdm);
}

///////////////////////////////////////////////////////////////////////////////
CAPIHook g_StartPage("Gdi32.dll", "StartPage",
   (PROC) Hook_StartPage);
CAPIHook g_StartDocA("Gdi32.dll", "StartDocA",
   (PROC) Hook_StartDocA);
CAPIHook g_StartDocW("Gdi32.dll", "StartDocW",
   (PROC) Hook_StartDocW);

//CAPIHook g_OpenPrinterA("Winspool.drv", "OpenPrinterA",
//   (PROC) Hook_OpenPrinterA);
//CAPIHook g_OpenPrinterW("Winspool.drv", "OpenPrinterW",
//   (PROC) Hook_OpenPrinterW);
//
CAPIHook g_CreateDCA("Gdi32.dll", "CreateDCA",
   (PROC) Hook_CreateDCA);
CAPIHook g_CreateDCW("Gdi32.dll", "CreateDCW",
   (PROC) Hook_CreateDCW);

HHOOK g_hhook = NULL;

///////////////////////////////////////////////////////////////////////////////


static LRESULT WINAPI GetMsgProc(int code, WPARAM wParam, LPARAM lParam) {
   return(CallNextHookEx(g_hhook, code, wParam, lParam));
}


///////////////////////////////////////////////////////////////////////////////


// Returns the HMODULE that contains the specified memory address
static HMODULE ModuleFromAddress(PVOID pv) {

   MEMORY_BASIC_INFORMATION mbi;
   return((VirtualQuery(pv, &mbi, sizeof(mbi)) != 0) 
      ? (HMODULE) mbi.AllocationBase : NULL);
}


///////////////////////////////////////////////////////////////////////////////


BOOL WINAPI LastMsgBoxInfo_HookAllApps(BOOL bInstall, DWORD dwThreadId) {

   BOOL bOk;

   if (bInstall) {

      chASSERT(g_hhook == NULL); // Illegal to install twice in a row

      // Install the Windows' hook
      g_hhook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, 
         ModuleFromAddress(LastMsgBoxInfo_HookAllApps), dwThreadId);

      bOk = (g_hhook != NULL);
   } else {

      chASSERT(g_hhook != NULL); // Can't uninstall if not installed
      bOk = UnhookWindowsHookEx(g_hhook);
      g_hhook = NULL;
   }

   return(bOk);
}


//////////////////////////////// End of File //////////////////////////////////
