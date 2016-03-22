#include "StdAfx.h"
#include "Util.h"


namespace file {

bool Exists(const TCHAR *filePath)
{
    if (NULL == filePath)
        return false;

    WIN32_FILE_ATTRIBUTE_DATA   fileInfo;
    BOOL res = GetFileAttributesEx(filePath, GetFileExInfoStandard, &fileInfo);
    if (0 == res)
        return false;

    if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return false;
    return true;
}

size_t GetSize(const TCHAR *filePath)
{
    HANDLE h = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,  
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,  NULL); 
    if (h == INVALID_HANDLE_VALUE)
        return INVALID_FILE_SIZE;

    // Don't use GetFileAttributesEx to retrieve the file size, as
    // that function doesn't interact well with symlinks, etc.
    LARGE_INTEGER lsize;
    BOOL ok = GetFileSizeEx(h, &lsize);
    CloseHandle(h);
    if (!ok)
        return INVALID_FILE_SIZE;

#ifdef _WIN64
    return lsize.QuadPart;
#else
    if (lsize.HighPart > 0)
        return INVALID_FILE_SIZE;
    return lsize.LowPart;
#endif
}

bool ReadAll(const TCHAR *filePath, char *buffer, size_t bufferLen)
{
    HANDLE h = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,  
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,  NULL); 
    if (h == INVALID_HANDLE_VALUE)
        return false;

    DWORD sizeRead;
    BOOL ok = ReadFile(h, buffer, (DWORD)bufferLen, &sizeRead, NULL);
    CloseHandle(h);

    return ok && sizeRead == bufferLen;
}

bool WriteAll(const TCHAR *filePath, void *data, size_t dataLen)
{
    HANDLE h = CreateFile(filePath, GENERIC_WRITE, FILE_SHARE_READ, NULL,  
                          CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,  NULL); 
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD size;
    BOOL ok = WriteFile(h, data, (DWORD)dataLen, &size, NULL);
    CloseHandle(h);

    return ok && dataLen == (size_t)size;
}

// Return true if the file wasn't there or was successfully deleted
bool Delete(const TCHAR *filePath)
{
    BOOL ok = DeleteFile(filePath);
    if (ok)
        return true;
    DWORD err = GetLastError();
    return ((ERROR_PATH_NOT_FOUND == err) || (ERROR_FILE_NOT_FOUND == err));
}

FILETIME GetModificationTime(const TCHAR *filePath)
{
    FILETIME lastMod = { 0 };
    HANDLE h = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,  
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,  NULL); 
    if (h != INVALID_HANDLE_VALUE)
        GetFileTime(h, NULL, NULL, &lastMod);
    CloseHandle(h);
    return lastMod;
}

}

void GetCurrentTimeString( TCHAR *wszTime, UINT nLen )
{
	struct tm *newtime;
	TCHAR am_pm[] = L"AM";
	__time64_t long_time;

	_time64( &long_time );           // Get time as 64-bit integer.
										// Convert to local time.
	newtime = _localtime64( &long_time ); // C4996
	// Note: _localtime64 deprecated; consider _localetime64_s

	if( newtime->tm_hour > 12 )        // Set up extension.
			_tcscpy_s( am_pm, 3, L"PM" );
	if( newtime->tm_hour > 12 )        // Convert from 24-hour
			newtime->tm_hour -= 12;    //   to 12-hour clock.
	if( newtime->tm_hour == 0 )        // Set hour to 12 if midnight.
			newtime->tm_hour = 12;

	_stprintf(wszTime, L"%4d-%02d-%02d %02d:%02d:%02d",
		newtime->tm_year + 1900, newtime->tm_mon+1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec );
}

void DumpPropVariant(PROPVARIANT *pPropVar, TCHAR* wszVar, INT nLen)
{
	DWORD dwRet = 0;
	// Don't iterate arrays, just inform as an array.
	if(pPropVar->vt & VT_ARRAY) {
		printf("(Array)\n");
		return;
	}

	// Don't handle byref for simplicity, just inform byref.
	if(pPropVar->vt & VT_BYREF) {
		printf("(ByRef)\n");
		return;
	}
	switch(pPropVar->vt) {
	case VT_EMPTY:
		_stprintf(wszVar, L"(EMPTY)");
		break;
	case VT_NULL:
		_stprintf(wszVar, L"(NULL)");
		break;
	case VT_BLOB:
		_stprintf(wszVar, L"(BLOB)");
		break;
	case VT_BOOL:
		_stprintf(wszVar, L"%s",
		pPropVar->boolVal ? "TRUE" : "FALSE");
		break;
	case VT_I2: // 2-byte signed int.
		_stprintf(wszVar, L"%d ", (int)pPropVar->iVal);
		break;
	case VT_I4: // 4-byte signed int.
		_stprintf(wszVar, L"%d", (int)pPropVar->lVal);
		break;
	case VT_R4: // 4-byte real.
		_stprintf(wszVar, L"%.2lf", (double)pPropVar->fltVal);
		break;
	case VT_R8: // 8-byte real.
		_stprintf(wszVar, L"%.2lf", (double)pPropVar->dblVal);
		break;
	case VT_BSTR:
	{ 
		// OLE Automation string.
		_stprintf(wszVar, L"%s", pPropVar->bstrVal);
	} 
	break;
	case VT_LPSTR: 
	{
		INT nLen = 0;

		nLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pPropVar->pszVal, -1, NULL, NULL);
		if( !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pPropVar->pszVal, strlen(pPropVar->pszVal)+1, wszVar, nLen))
			dwRet = GetLastError();

		if (dwRet == ERROR_NO_UNICODE_TRANSLATION) {
			nLen = MultiByteToWideChar(52936, 0, pPropVar->pszVal, -1, NULL, NULL);
			if( !MultiByteToWideChar(52936, 0, pPropVar->pszVal, -1, wszVar, nLen))
				dwRet = GetLastError();
		}
	} 
	break;
	case VT_LPWSTR: 
	{ 
		// Null-terminated string.
		_stprintf(wszVar, L"%s", pPropVar->pwszVal);
	} 
	break;
	case VT_FILETIME: 
	{
		SYSTEMTIME lst;
		FILETIME lft;
		TCHAR *dayPre[] = {L"Sun",L"Mon",L"Tue",L"Wed",L"Thu",L"Fri",L"Sat"};
		FileTimeToLocalFileTime(&pPropVar->filetime, &lft);                
		FileTimeToSystemTime(&lft, &lst);
		_stprintf(wszVar, L"%02d:%02d.%02d %s, %s %02d-%02d-%d",
				1+(lst.wHour-1)%12, lst.wMinute, lst.wSecond,(lst.wHour>=12) ? L"PM" : L"AM", 
				dayPre[lst.wDayOfWeek%7],lst.wMonth, lst.wDay, lst.wYear);
	} 
	break;
	case VT_CF: // Clipboard format.
		_stprintf(wszVar, L"(Clipboard format)");
		break;
	default: // Unhandled type, consult wtypes.h's VARENUM structure.
		_stprintf(wszVar, L"(Unhandled type: 0x%08lx)", pPropVar->vt);
		break;
	}
}

std::string ToUtf8(const std::wstring& str)
{
	const wchar_t* buffer = str.c_str();
	int len = str.size();
    int nChars = ::WideCharToMultiByte(
            CP_UTF8,
            0,
            buffer,
            len,
            NULL,
            0,
            NULL,
            NULL);
    if (nChars == 0) return "";

    std::string newbuffer;
    newbuffer.resize(nChars) ;
    ::WideCharToMultiByte(
            CP_UTF8,
            0,
            buffer,
            len,
            const_cast< char* >(newbuffer.c_str()),
            nChars,
            NULL,
            NULL); 

    return newbuffer;
}
