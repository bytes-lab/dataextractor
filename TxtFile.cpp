#include "StdAfx.h"

#include "Util.h"
#include "TxtFile.h"

#ifdef SUPPORT_TXT

enum MarkupDocFlags
{
	TXT_UTF16LEFILE = 1,
	TXT_UTF8PREAMBLE = 4,
	TXT_IGNORECASE = 8,
	TXT_UTF16BEFILE = 16,
	TXT_TRIMWHITESPACE = 32,
	TXT_COLLAPSEWHITESPACE = 64
};

struct BOM_TABLE 
{ 
	CONST CHAR* pszBom; 
	INT nBomLen; 
	TCHAR* pszBomEnc; 
	INT nBomFlag; 
} btUtfIds[] = {
	{ "\xef\xbb\xbf", 3, L"UTF-8", TXT_UTF8PREAMBLE },
	{ "\xff\xfe", 2, L"UTF-16LE", TXT_UTF16LEFILE },
	{ "\xfe\xff", 2, L"UTF-16BE", TXT_UTF16BEFILE },
	{ NULL,0,NULL,0 } 
};

CTxtFile::CTxtFile(void)
{
	ZeroMemory(m_wszDateEdited, MAX_META_LEN * sizeof(TCHAR));
}

CTxtFile::~CTxtFile(void)
{
}

BOOL CTxtFile::Load(TCHAR* wszFilePath)
{
	/*******************Read modified date***********************/
	SYSTEMTIME stEdited;
	FILETIME ftEdited = file::GetModificationTime( wszFilePath );

	::FileTimeToSystemTime(&ftEdited, &stEdited);
	_stprintf(m_wszDateEdited, L"%4d-%02d-%02d %02d:%02d:%02d", stEdited.wYear, stEdited.wMonth, stEdited.wDay, stEdited.wHour, stEdited.wMinute, stEdited.wSecond);

#ifdef CONTENT_EXTRACTION
	/*************************Read content***********************/
	FILE *fpDoc = _wfopen(wszFilePath, L"rb" );
	if ( ! fpDoc )
		return FALSE;

	// Prepare file
	BOOL bSuccess = TRUE;
	INT nBomLen = 0;
	INT nDocFlags = 0;
	INT nFileByteLen, nFileCharUnitSize = 1; // unless UTF-16 BOM
	wstring strEncoding;

	// Get file length
	fseek( fpDoc, 0, SEEK_END );
	nFileByteLen = ftell( fpDoc );
	fseek( fpDoc, 0, SEEK_SET );

	// Read the top of the file to check BOM and encoding
	INT nReadTop = 1024;
	if ( nFileByteLen < nReadTop )
		nReadTop = (INT)nFileByteLen;
	if ( nReadTop )
	{
		CHAR* pFileTop = new CHAR[nReadTop];
		if ( nReadTop )
			bSuccess = ( fread( pFileTop, nReadTop, 1, fpDoc ) == 1 );
		if ( bSuccess )
		{
			// Check for Byte Order Mark (preamble)
			int nBomCheck = 0;
			nDocFlags &= ~( TXT_UTF16LEFILE | TXT_UTF8PREAMBLE );
			while ( btUtfIds[nBomCheck].pszBom )
			{
				while ( nBomLen < btUtfIds[nBomCheck].nBomLen )
				{
					if ( nBomLen >= nReadTop || pFileTop[nBomLen] != btUtfIds[nBomCheck].pszBom[nBomLen] )
						break;
					++nBomLen;
				}
				if ( nBomLen == btUtfIds[nBomCheck].nBomLen )
				{
					nDocFlags |= btUtfIds[nBomCheck].nBomFlag;
					if ( nBomLen == 2 )
						nFileCharUnitSize = 2;
					strEncoding = btUtfIds[nBomCheck].pszBomEnc;
					break;
				}
				++nBomCheck;
				nBomLen = 0;
			}
			if ( nReadTop > nBomLen )
				fseek( fpDoc, nBomLen, SEEK_SET );
		} else
			return bSuccess;
		delete [] pFileTop;
	}

	UINT nOpFileByteLen = 0;
	UINT nFileByteOffset = (LONG)nBomLen;

	// Read text content according to the encoding
	LONG nBytesRemaining = nFileByteLen - nFileByteOffset;
	if ( nDocFlags & (TXT_UTF16LEFILE | TXT_UTF16BEFILE) )
	{
		WCHAR* pUTF16Buffer = new WCHAR[nBytesRemaining/2 + nBytesRemaining/100];
		bSuccess = fread( pUTF16Buffer, nBytesRemaining, 1, fpDoc );
		pUTF16Buffer[nBytesRemaining/2] = L'\0';
		this->m_wszContent = pUTF16Buffer;
		delete[] pUTF16Buffer;
	}
	else // single or multibyte file (i.e. not UTF-16)
	{
		char* pBuffer = new char[nBytesRemaining];
		bSuccess = fread( pBuffer, nBytesRemaining, 1, fpDoc );
		pBuffer[nBytesRemaining] = '\0';
		INT nLen = 0;
		nLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pBuffer, -1, NULL, NULL);
		WCHAR* pwszUTF16 = new WCHAR[nLen];
		nLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pBuffer, nBytesRemaining, pwszUTF16, nLen);
		pwszUTF16[nLen] = L'\0';
		this->m_wszContent = pwszUTF16;
		delete[] pwszUTF16;
	}

	fclose(fpDoc);
#endif

	return TRUE;
}

JSONValue* CTxtFile::Serialize(JSONObject *jsonNone)
{
	JSONObject *jsonMeta = new JSONObject;

	(*jsonMeta)[L"DATE EDITED"] = new JSONValue(m_wszDateEdited);

	return CDEFile::Serialize(jsonMeta);
}
#endif