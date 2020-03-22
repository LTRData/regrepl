#define WIN32_LEAN_AND_MEAN
#include <winstrct.h>
#include <wstring.h>

#include "regrepl.rc.h"

#ifdef _DLL
#ifdef _M_IX86
#pragma comment(lib, "minwcrt.lib")
#endif
#endif
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

HINSTANCE hInstance = NULL;

bool bCancel = false;
bool bSilent = false;
bool bRunningOnWin32s = false;

char cKeyPath[65535] = "";
HWND hwndProgressDialog = NULL;
char cFrom[260] = "";
char cTo[260] = "";
WCHAR wcFrom[260] = L"";
WCHAR wcTo[260] = L"";
size_t iFromLen = 0;
size_t iToLen = 0;
DWORD dwReplaceCounter = 0;
DWORD dwReplaceErrorCounter = 0;
DWORD dwErrorCounter = 0;

char cValueName[260];
char cValueData[65535];

void
UpdateStatus()
{
  static char buf[260];
  _snprintf(buf, sizeof buf,
	    "Replacements: %u, Replacement errors: %u, Key errors: %u, "
	    "searching...", dwReplaceCounter, dwReplaceErrorCounter,
	    dwErrorCounter);
  buf[sizeof(buf)-1] = 0;
  SetDlgItemText(hwndProgressDialog, IDC_STATICSTATUS, buf);
}

void
DisplayWarning(LONG lErrNo, LPCSTR lpMsg)
{
  static char msg[260] = "";

  if (bSilent)
    return;

  WErrMsg errmsg(lErrNo);
  _snprintf(msg, sizeof msg,
	    "%s '%s':\nError %u: %s\nDisplay further error messages?", lpMsg,
	    cKeyPath, lErrNo, (LPCSTR)errmsg);
  msg[sizeof(msg)-1] = 0;
  switch (MessageBox(hwndProgressDialog, msg, "Error",
		     MB_ICONEXCLAMATION | MB_YESNOCANCEL))
    {
    case IDNO:
      bSilent = true;
      return;
    case IDCANCEL:
      bCancel = true;
    }
}

bool
DoEvents()
{
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if (!IsDialogMessage(hwndProgressDialog, &msg))
	TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

  return !bCancel;
}

void
ReplaceInString(HKEY hKey, DWORD dwType, DWORD dwSize)
{
  static char cNewData[65536];
  switch (dwType)
    {
    case REG_SZ: case REG_EXPAND_SZ:
      {
	bool bDone = false;
	char *oldplace = cValueData;
	char *newplace = cNewData;
	for (;;)
	  {
	    char *nextplace = strstr(oldplace, cFrom);
	    if (nextplace == NULL)
	      {
		// Nothing done? Just break.
		if (!bDone)
		  break;

		size_t iBufSizeLeft = sizeof(cNewData)-(newplace-cNewData);
		size_t iLen = strlen(oldplace);
		if (iLen >= iBufSizeLeft)
		  {
		    _snprintf(cValueData, sizeof cValueData,
			      "Data too big after replacements: '%s\\%s'",
			      cKeyPath, cValueName);
		    cValueData[sizeof(cValueData)-1] = 0;
		    MessageBox(hwndProgressDialog, cValueData, "Error",
			       MB_ICONEXCLAMATION);
		    break;
		  }
		  
		strcpy(newplace, oldplace);
		newplace += iLen;
		cNewData[sizeof(cNewData)-1] = 0;
		LONG lErrNo =
		  RegSetValueEx(hKey, cValueName, 0, dwType, (LPBYTE)cNewData,
				(DWORD) (newplace - cNewData + 1));
		if (lErrNo != NO_ERROR)
		  {
		    DisplayWarning(lErrNo, "Error saving value in key");
		    dwReplaceErrorCounter++;
		  }
		else
		  dwReplaceCounter++;

		UpdateStatus();
		    
		break;
	      }

	    bDone = true;
	    nextplace[0] = 0;
	    size_t iBufSizeLeft = sizeof(cNewData)-(newplace-cNewData);
	    size_t iLen = strlen(oldplace);
	    if (iLen + iToLen >= iBufSizeLeft)
	      {
		_snprintf(cValueData, sizeof cValueData,
			  "Data too big after replacements: '%s\\%s'",
			  cKeyPath, cValueName);
		cValueData[sizeof(cValueData)-1] = 0;
		MessageBox(hwndProgressDialog, cValueData, "Error",
			   MB_ICONEXCLAMATION);
		break;
	      }
	    strcpy(newplace, oldplace);
	    newplace += iLen;
	    strcpy(newplace, cTo);
	    newplace += iToLen;
	    oldplace += iLen + iFromLen;
	  }
	break;
      }
    case REG_BINARY:
      {
	if (memstr(cValueData, cFrom, dwSize) == NULL)
	  {
	    bool bDone = false;
	    char *oldplace = cValueData;
	    char *newplace = cNewData;

	    if (wcFrom[0] == 0)
	      {
		if (MultiByteToWideChar(CP_ACP, 0, cFrom, -1, wcFrom,
					sizeof(wcFrom)/sizeof(*wcFrom)) == 0)
		  break;
		if (MultiByteToWideChar(CP_ACP, 0, cTo, -1, wcTo,
					sizeof(wcTo)/sizeof(*wcTo)) == 0)
		  break;
	      }

	    for (;;)
	      {
		char *nextplace =
		  (char*)memwcs((LPWSTR)oldplace, wcFrom,
				(dwSize - (oldplace-cValueData)) >> 1);
		if (nextplace == NULL)
		  {
		    // Nothing done? Just break.
		    if (!bDone)
		      break;

		    size_t iBufSizeLeft = sizeof(cNewData)-(newplace-cNewData);
		    size_t iLen = dwSize - (oldplace-cValueData);
		    if (iLen > iBufSizeLeft)
		      {
			_snprintf(cValueData, sizeof cValueData,
				  "Data too big after replacements: '%s\\%s'",
				  cKeyPath, cValueName);
			cValueData[sizeof(cValueData)-1] = 0;
			MessageBox(hwndProgressDialog, cValueData, "Error",
				   MB_ICONEXCLAMATION);
			break;
		      }
		  
		    memcpy(newplace, oldplace, iLen);
		    newplace += iLen;
		    LONG lErrNo =
		      RegSetValueEx(hKey, cValueName, 0, dwType,
				    (LPBYTE) cNewData,
				    (DWORD) (newplace - cNewData));
		    if (lErrNo != NO_ERROR)
		      {
			DisplayWarning(lErrNo, "Error saving value in key");
			dwReplaceErrorCounter++;
		      }
		    else
		      dwReplaceCounter++;

		    UpdateStatus();
		    
		    break;
		  }

		bDone = true;
		size_t iBufSizeLeft = sizeof(cNewData)-(newplace-cNewData);
		size_t iLen = nextplace - oldplace;
		if (iLen + iToLen > iBufSizeLeft)
		  {
		    _snprintf(cValueData, sizeof cValueData,
			      "Data too big after replacements: '%s\\%s'",
			      cKeyPath, cValueName);
		    cValueData[sizeof(cValueData)-1] = 0;
		    MessageBox(hwndProgressDialog, cValueData, "Error",
			       MB_ICONEXCLAMATION);
		    break;
		  }
		memcpy(newplace, oldplace, iLen);
		newplace += iLen;
		memcpy(newplace, wcTo, (iToLen << 1));
		newplace += (iToLen << 1);
		oldplace += iLen + (iFromLen << 1);
	      }
	    break;
	  }

	bool bDone = false;
	char *oldplace = cValueData;
	char *newplace = cNewData;
	for (;;)
	  {
	    char *nextplace = memstr(oldplace, cFrom,
				     dwSize - (oldplace-cValueData));
	    if (nextplace == NULL)
	      {
		// Nothing done? Just break.
		if (!bDone)
		  break;

		size_t iBufSizeLeft = sizeof(cNewData)-(newplace-cNewData);
		size_t iLen = dwSize - (oldplace-cValueData);
		if (iLen > iBufSizeLeft)
		  {
		    _snprintf(cValueData, sizeof cValueData,
			      "Data too big after replacements: '%s\\%s'",
			      cKeyPath, cValueName);
		    cValueData[sizeof(cValueData)-1] = 0;
		    MessageBox(hwndProgressDialog, cValueData, "Error",
			       MB_ICONEXCLAMATION);
		    break;
		  }
		  
		memcpy(newplace, oldplace, iLen);
		newplace += iLen;
		LONG lErrNo =
		  RegSetValueEx(hKey, cValueName, 0, dwType, (LPBYTE) cNewData,
				(DWORD) (newplace - cNewData));
		if (lErrNo != NO_ERROR)
		  {
		    DisplayWarning(lErrNo, "Error saving value in key");
		    dwReplaceErrorCounter++;
		  }
		else
		  dwReplaceCounter++;

		UpdateStatus();
		    
		break;
	      }

	    bDone = true;
	    size_t iBufSizeLeft = sizeof(cNewData)-(newplace-cNewData);
	    size_t iLen = nextplace - oldplace;
	    if (iLen + iToLen > iBufSizeLeft)
	      {
		_snprintf(cValueData, sizeof cValueData,
			  "Data too big after replacements: '%s\\%s'",
			  cKeyPath, cValueName);
		cValueData[sizeof(cValueData)-1] = 0;
		MessageBox(hwndProgressDialog, cValueData, "Error",
			   MB_ICONEXCLAMATION);
		break;
	      }
	    memcpy(newplace, oldplace, iLen);
	    newplace += iLen;
	    memcpy(newplace, cTo, iToLen);
	    newplace += iToLen;
	    oldplace += iLen + iFromLen;
	  }
	break;
      }
    case REG_MULTI_SZ:
      {
	bool bDone = false;
	char *oldplace = cValueData;
	char *newplace = cNewData;
	for (;;)
	  {
	    char *nextplace = strstr(oldplace, cFrom);
	    if (nextplace == NULL)
	      {
		size_t iBufSizeLeft = sizeof(cNewData)-(newplace-cNewData);
		size_t iLen = strlen(oldplace);
		if (iLen >= iBufSizeLeft - 1)
		  {
		    _snprintf(cValueData, sizeof cValueData,
			      "Data too big after replacements: '%s\\%s'",
			      cKeyPath, cValueName);
		    cValueData[sizeof(cValueData)-1] = 0;
		    MessageBox(hwndProgressDialog, cValueData, "Error",
			       MB_ICONEXCLAMATION);
		    break;
		  }
		  
		strcpy(newplace, oldplace);
		oldplace += iLen + 1;
		newplace += iLen + 1;
		if (oldplace[0] == 0)
		  {
		    if (!bDone)
		      break;

		    cNewData[sizeof(cNewData)-2] = 0;
		    cNewData[sizeof(cNewData)-1] = 0;
		    LONG lErrNo =
		      RegSetValueEx(hKey, cValueName, 0, dwType,
				    (LPBYTE) cNewData,
				    (DWORD) (newplace - cNewData + 1));
		    if (lErrNo != NO_ERROR)
		      {
			DisplayWarning(lErrNo,
				       "Error saving value in key");
			dwReplaceErrorCounter++;
		      }
		    else
		      dwReplaceCounter++;

		    UpdateStatus();
		  
		    break;
		  }
	      }
	    else
	      {
		bDone = true;
		nextplace[0] = 0;
		size_t iBufSizeLeft = sizeof(cNewData)-(newplace-cNewData);
		size_t iLen = strlen(oldplace);
		if (iLen + iToLen >= iBufSizeLeft - 1)
		  {
		    _snprintf(cValueData, sizeof cValueData,
			      "Data too big after replacements: '%s\\%s'",
			      cKeyPath, cValueName);
		    cValueData[sizeof(cValueData)-1] = 0;
		    MessageBox(hwndProgressDialog, cValueData, "Error",
			       MB_ICONEXCLAMATION);
		    break;
		  }
		strcpy(newplace, oldplace);
		newplace += iLen;
		strcpy(newplace, cTo);
		newplace += iToLen;
		oldplace += iLen + iFromLen;
	      }
	  }
	break;
      }
    }
}

void
DoSubKey(HKEY hKey, LPSTR lpszCurrentKey)
{
  SetDlgItemText(hwndProgressDialog, IDC_STATIC_CURKEY, cKeyPath);

  DWORD dwType;
  LONG lErrNo;

  for (DWORD dwIndex = 0; ; dwIndex++)
    {
      if (bRunningOnWin32s)
	{
	  LONG lDataSize = sizeof cValueData;
	  lErrNo = RegQueryValue(hKey, NULL, cValueData, &lDataSize);
	  if (lErrNo != NO_ERROR)
	    {
	      DisplayWarning(lErrNo, "Error getting value of key");
	      dwErrorCounter++;
	      UpdateStatus();
	      break;
	    }

	  cValueName[0] = 0;

	  DoEvents();

	  if (bCancel)
	    return;

	  if ((size_t) lDataSize <= iFromLen)
	    break;

	  ReplaceInString(hKey, REG_SZ, lDataSize);
	  break;
	}

      DWORD dwNameSize = sizeof cValueName;
      DWORD dwDataSize = sizeof cValueData;

      lErrNo = RegEnumValue(hKey, dwIndex, cValueName, &dwNameSize, NULL,
			    &dwType, (LPBYTE)cValueData, &dwDataSize);
      if (lErrNo == ERROR_NO_MORE_ITEMS)
	break;

      DoEvents();

      if (bCancel)
	return;

      if (lErrNo != NO_ERROR)
	{
	  DisplayWarning(lErrNo, "Error enumerating values in key");
	  dwErrorCounter++;
	  UpdateStatus();
	  break;
	}

      if (dwDataSize > (DWORD)iFromLen)
	ReplaceInString(hKey, dwType, dwDataSize);
    }

  for (DWORD dwIndex = 0; ; dwIndex++)
    {
      lErrNo = RegEnumKey(hKey, dwIndex, cValueName, sizeof cValueName);
      if ((lErrNo == ERROR_NO_MORE_ITEMS) | (lErrNo == ERROR_FILE_NOT_FOUND))
	break;

      DoEvents();

      if (bCancel)
	return;

      if (lErrNo != NO_ERROR)
	{
	  DisplayWarning(lErrNo, "Cannot enumerate subkeys under key");
	  UpdateStatus();
	  break;
	}

      if (sizeof(cKeyPath)-(lpszCurrentKey - cKeyPath) > 4)
	{
	  _snprintf(lpszCurrentKey,
		    sizeof(cKeyPath)-(lpszCurrentKey - cKeyPath)-3,
		    "\\%s", cValueName);
	  cKeyPath[sizeof(cKeyPath)-1] = 0;
	}
      HKEY hKeySubKey;
      lErrNo = RegOpenKey(hKey, cValueName, &hKeySubKey);
      if (lErrNo != NO_ERROR)
	{
	  DisplayWarning(lErrNo, "Cannot open key");
	  dwErrorCounter++;
	  lpszCurrentKey[0] = 0;
	  UpdateStatus();
	  continue;
	}

      DoSubKey(hKeySubKey, lpszCurrentKey+strlen(lpszCurrentKey));
      RegCloseKey(hKeySubKey);
      lpszCurrentKey[0] = 0;
      if (bCancel)
	return;
    }
}

INT_PTR CALLBACK
ProgressDlgProc(HWND /*hWnd*/, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
  if ((uMsg == WM_COMMAND) & (LOWORD(wParam) == IDCANCEL))
    {
      bCancel = true;
      return TRUE;
    }

  return FALSE;
}

INT_PTR CALLBACK
MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
  switch (uMsg)
    {
    case WM_INITDIALOG:
      SetClassLongPtr(hWnd, GCLP_HICON,
		      (LONG_PTR)
		      LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON)));

      if (bRunningOnWin32s)
	{
	  CheckDlgButton(hWnd, IDC_RADIOBUTTON_HKCR, BST_CHECKED);
	  EnableWindow(GetDlgItem(hWnd, IDC_STATICSERVER), FALSE);
	  EnableWindow(GetDlgItem(hWnd, IDC_EDITSERVER), FALSE);
	  EnableWindow(GetDlgItem(hWnd, IDC_RADIOBUTTON_HKLM), FALSE);
	  EnableWindow(GetDlgItem(hWnd, IDC_RADIOBUTTON_HKCU), FALSE);
	  EnableWindow(GetDlgItem(hWnd, IDC_RADIOBUTTON_HKU), FALSE);
	}
      else
	CheckDlgButton(hWnd, IDC_RADIOBUTTON_HKLM, BST_CHECKED);

      SendDlgItemMessage(hWnd, IDC_EDITFROM, EM_LIMITTEXT,
			 (WPARAM)sizeof(cFrom) - 1, 0);
      SendDlgItemMessage(hWnd, IDC_EDITTO, EM_LIMITTEXT,
			 (WPARAM)sizeof(cTo) - 1, 0);
      SendDlgItemMessage(hWnd, IDC_EDITSERVER, EM_LIMITTEXT,
			 (WPARAM)sizeof(cFrom) - 3, 0);
      SendDlgItemMessage(hWnd, IDC_EDITSUBKEY, EM_LIMITTEXT,
			 (WPARAM)sizeof(cFrom) - 1, 0);

      return TRUE;
    case WM_CLOSE:
      EndDialog(hWnd, 0);
      return TRUE;
    case WM_COMMAND:
      if (GetWindowTextLength(GetDlgItem(hWnd, IDC_EDITFROM)) > 0)
	EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
      else
	EnableWindow(GetDlgItem(hWnd, IDOK), FALSE);

      switch(LOWORD(wParam))
	{
	case IDHELP:
	  {
	    MessageBox(hWnd,
		       "Replace Registry Values Tool. Freeware by Olof "
		       "Lagerkvist 1998-2007.\r\n"
		       "http://www.ltr-data.se      olof@ltr-data.se\r\n"
		       "\n"
		       "For information about this program, including usage "
		       "and redistribution rights, read the file REGREPL.TXT "
		       "included in the same archive as the program file or "
		       "visit the web site.", "Registry Replace Tool",
		       MB_ICONINFORMATION);
	    return TRUE;
	  }
	case IDOK:
	  {
	    hwndProgressDialog = 
	      CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOGPROGRESS),
			   hWnd, ProgressDlgProc);

	    if (hwndProgressDialog == NULL)
	      {
		WErrMsg errmsg;
		MessageBox(hWnd, errmsg, "CreateDialog() failed.",
			   MB_ICONSTOP);
		return TRUE;
	      }

	    EnableWindow(hWnd, FALSE);

	    DoEvents();

	    HKEY hKeyRoot = NULL;

	    if (IsDlgButtonChecked(hWnd, IDC_RADIOBUTTON_HKCR) == BST_CHECKED)
	      hKeyRoot = HKEY_CLASSES_ROOT;
	    else if (IsDlgButtonChecked(hWnd, IDC_RADIOBUTTON_HKLM) ==
		     BST_CHECKED)
	      hKeyRoot = HKEY_LOCAL_MACHINE;
	    else if (IsDlgButtonChecked(hWnd, IDC_RADIOBUTTON_HKCU) ==
		     BST_CHECKED)
	      hKeyRoot = HKEY_CURRENT_USER;
	    else if (IsDlgButtonChecked(hWnd, IDC_RADIOBUTTON_HKU) ==
		     BST_CHECKED)
	      hKeyRoot = HKEY_USERS;

	    strcpy(cFrom, "\\\\");
	    if (GetDlgItemText(hWnd, IDC_EDITSERVER, cFrom+2, sizeof(cFrom)-2))
	      {
		SetDlgItemText(hwndProgressDialog, IDC_STATICSTATUS,
			       "Connecting...");
		LONG lErrNo = RegConnectRegistry(cFrom, hKeyRoot, &hKeyRoot);
		if (lErrNo != NO_ERROR)
		  {
		    WErrMsg errmsg(lErrNo);
		    MessageBox(hwndProgressDialog, errmsg,
			       "Error connecting to remote registry",
			       MB_ICONEXCLAMATION);
		    EnableWindow(hWnd, TRUE);
		    DestroyWindow(hwndProgressDialog);
		    return TRUE;
		  }
	      }

	    if (GetDlgItemText(hWnd, IDC_EDITSUBKEY, cFrom, sizeof cFrom))
	      {
		SetDlgItemText(hwndProgressDialog, IDC_STATICSTATUS,
			       "Opening...");
		LONG lErrNo = RegOpenKey(hKeyRoot, cFrom, &hKeyRoot);
		if (lErrNo != NO_ERROR)
		  {
		    if ((lErrNo == ERROR_FILE_NOT_FOUND) |
			(lErrNo == ERROR_PATH_NOT_FOUND))
		      MessageBox(hwndProgressDialog, "Key not found.",
				 "Error opening registry key",
				 MB_ICONEXCLAMATION);
		    else
		      {
			WErrMsg errmsg(lErrNo);
			MessageBox(hwndProgressDialog, errmsg,
				   "Error opening registry key",
				   MB_ICONEXCLAMATION);
		      }
		    EnableWindow(hWnd, TRUE);
		    DestroyWindow(hwndProgressDialog);
		    return TRUE;
		  }
	      }

	    GetDlgItemText(hWnd, IDC_EDITTO, cTo, sizeof cTo);
	    // If no subkey specified and to text to replace with, warning...
	    if ((cTo[0] == 0) & (cFrom[0] == 0))
	      if (MessageBox(hwndProgressDialog,
			     "You have not typed any text to replace with. "
			     "Are you sure you want to delete the text when "
			     "found in any value in the specified root key or "
			     "any subkeys?",
			     "Warning", MB_ICONEXCLAMATION | MB_YESNO) == IDNO)
		{
		  EnableWindow(hWnd, TRUE);
		  DestroyWindow(hwndProgressDialog);
		  return TRUE;
		}

	    if (GetDlgItemText(hWnd, IDC_EDITFROM, cFrom, sizeof cFrom) == 0)
	      {
		WErrMsg errmsg;
		MessageBox(hwndProgressDialog, errmsg,
			   "GetDlgItemText() failed.", MB_ICONSTOP);
		EnableWindow(hWnd, TRUE);
		DestroyWindow(hwndProgressDialog);
		return TRUE;
	      }

	    EnableWindow(GetDlgItem(hwndProgressDialog, IDCANCEL), TRUE);
	    SetFocus(GetDlgItem(hwndProgressDialog, IDCANCEL));
	    iFromLen = strlen(cFrom);
	    iToLen = strlen(cTo);
	    cKeyPath[0] = 0;
	    // We don't convert to Unicode until we need.
	    wcFrom[0] = 0;
	    SetDlgItemText(hwndProgressDialog, IDC_STATICSTATUS,
			   "Searching...");
	    bCancel = false;
	    bSilent = false;
	    dwReplaceCounter = 0;
	    dwReplaceErrorCounter = 0;
	    dwErrorCounter = 0;

	    DoSubKey(hKeyRoot, cKeyPath);

	    SetDlgItemText(hwndProgressDialog, IDCANCEL, "&Close");
	    _snprintf(cFrom, sizeof cFrom,
		      "Replacements: %u, Replacement errors: %u, "
		      "Key errors: %u. %s.", dwReplaceCounter,
		      dwReplaceErrorCounter, dwErrorCounter,
		      bCancel ? "Cancelled" : "Done");
	    cFrom[sizeof(cFrom)-1] = 0;
	    SetDlgItemText(hwndProgressDialog, IDC_STATICSTATUS, cFrom);
	    SetDlgItemText(hwndProgressDialog, IDC_STATIC_CURKEY, "");
	    bCancel = false;
	    while (!bCancel)
	      {
		WaitMessage();
		DoEvents();
	      }

	    EnableWindow(hWnd, TRUE);
	    DestroyWindow(hwndProgressDialog);

	    RegCloseKey(hKeyRoot);

	    return TRUE;
	  }
	case IDC_EDITTO:
	  {
	    if (HIWORD(wParam) != EN_SETFOCUS)
	      return TRUE;

	    char buf[260] = "";
	    if (GetWindowTextLength(GetDlgItem(hWnd, IDC_EDITTO)) == 0)
	      {
		GetDlgItemText(hWnd, IDC_EDITFROM, buf, sizeof buf);
		SetDlgItemText(hWnd, IDC_EDITTO, buf);
		PostMessage(GetDlgItem(hWnd, IDC_EDITTO), EM_SETSEL, 0, -1);
	      }
	    return TRUE;
	  }
	}
      return FALSE;
    }

  return FALSE;
}

int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE, LPSTR lpCmdLine, int)
{
  if (*lpCmdLine != 0)
    {
      MessageBox(NULL,
		 "Replace Registry Values Tool. Freeware by "
		 "Olof Lagerkvist 1998-2007.\r\nhttp://www.ltr-data.se"
		 "\r\n\nFor information about this program, "
		 "including usage and redistribution rights, "
		 "read the file REGREPL.TXT included in the "
		 "same archive as the program file or visit the web site.\r\n"
		 "\nThis is a Windows GUI application and it doesn't "
		 "accept any command line switches or parameters.",
		 "Registry Replace Tool", MB_ICONINFORMATION | MB_TASKMODAL);
      return 1;
    }

  hInstance = hInst;
  bRunningOnWin32s = WinVer_Win32s;

  if (DialogBox(hInst, MAKEINTRESOURCE(IDD_MAINDLG), NULL, MainDlgProc) == -1)
    {
      WErrMsg errmsg;
      MessageBox(NULL, errmsg, "Cannot display main window",
		 MB_ICONSTOP | MB_TASKMODAL);
      return 0;
    }

  return 1;
}
