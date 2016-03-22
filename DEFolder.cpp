#include "StdAfx.h"

#include "DEFolder.h"
#include "DocFile.h"
#include "PptFile.h"
#include "XlsFile.h"
#include "DocxFile.h"
#include "PptxFile.h"
#include "XlsxFile.h"
#include "PdfFile.h"
#include "TxtFile.h"
#include "RtfFile.h"
#include "JpegFile.h"
#include "TiffFile.h"
#include "BmpFile.h"

CDEFolder::CDEFolder(void)
{
	m_aEBooks.clear();
	m_aSubFolders.clear();
}

CDEFolder::~CDEFolder(void)
{
}

BOOL CDEFolder::Load(TCHAR* wszFolderPath)
{
	CDEFolder *pSubFolder = NULL;
	
	TCHAR				wszTmpFolderPath[MAX_PATH], wszBuf[MAX_PATH];
	INT					nPathLen;
	INT					nSlashLen;
	HANDLE				hFile;
	WIN32_FIND_DATA		FindData;

	INT nPatternLen;
	TCHAR cChar, *wszPattern;

	// store folder path
	_tcscpy_s(m_wszFolderPath, MAX_PATH, wszFolderPath);

	// set temp folder path to be used
	_tcscpy_s(wszTmpFolderPath, MAX_PATH, wszFolderPath);

	nPathLen = _tcslen(wszTmpFolderPath);
	wszPattern = L"*.*";
	nPatternLen = _tcslen(wszPattern);

	/*
	 * Must put backslash between wszTmpFolderPath and wszPattern, unless
	 * last character of wszTmpFolderPath is slash or colon.
	 *
	 *   'dir' => 'dir\*'
	 *   'dir\' => 'dir\*'
	 *   'dir/' => 'dir/*'
	 *   'c:' => 'c:*'
	 *
	 * 'c:*' and 'c:\*' are different files!
	 */
	cChar = wszTmpFolderPath[nPathLen - 1];
	if (/*cChar == ':' || */cChar == '/' || cChar == '\\')
		nSlashLen = nPathLen;
	else
		nSlashLen = nPathLen + 1;
 
	hFile = INVALID_HANDLE_VALUE;
 
	/* Append backslash + wszPattern + \0 to wszTmpFolderPath. */
	wszTmpFolderPath[nPathLen] = '\\';
	_tcsncpy_s(wszTmpFolderPath + nSlashLen, MAX_PATH-nSlashLen,
		wszPattern, nPatternLen + 1);
 
	/* Find all files to match wszPattern. */
	hFile = FindFirstFile(wszTmpFolderPath, &FindData);
	if (hFile == INVALID_HANDLE_VALUE) {
		/* Check if no files match wszPattern. */
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
			goto subdirs;

		if (GetLastError() == ERROR_ACCESS_DENIED) {
			_tprintf(L"A folder access denied: [%s]\n", wszFolderPath);
			return TRUE;
		}

		if (GetLastError() == ERROR_PATH_NOT_FOUND)
			_tprintf(L"The folder path is incorrect: [%s]\n", wszFolderPath);
 
		/* Bail out from other errors. */
		wszTmpFolderPath[nPathLen] = '\0';
		goto error;
	}

	/* Remove wszPattern from wszTmpFolderPath; keep backslash. */
	wszTmpFolderPath[nSlashLen] = '\0';
 
	/* Print all files to match wszPattern. */
	do {
		_stprintf_s(wszBuf, MAX_PATH, L"%ls%ls", wszTmpFolderPath, FindData.cFileName);
		if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 5, L".docx"))
		{
#ifdef SUPPORT_DOCX
			CDocxFile *pEBook = new CDocxFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".docx");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 5, L".pptx"))
		{
#ifdef SUPPORT_PPTX
			CPptxFile *pEBook = new CPptxFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".pptx");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 5, L".xlsx"))
		{
#ifdef SUPPORT_XLSX
			CXlsxFile *pEBook = new CXlsxFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".xlsx");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 4, L".doc"))
		{
#ifdef SUPPORT_DOC
			CDocFile *pEBook = new CDocFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".doc");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		} 
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 4, L".ppt"))
		{
#ifdef SUPPORT_PPT
			CPptFile *pEBook = new CPptFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".ppt");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 4, L".xls"))
		{
#ifdef SUPPORT_XLS
			CXlsFile *pEBook = new CXlsFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".xls");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 4, L".pdf"))
		{
#ifdef SUPPORT_PDF
			CPdfFile *pEBook = new CPdfFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".pdf");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 4, L".txt"))
		{
#ifdef SUPPORT_TXT
			CTxtFile *pEBook = new CTxtFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".txt");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 4, L".rtf"))
		{
#ifdef SUPPORT_RTF
			CRtfFile *pEBook = new CRtfFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".rtf");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 4, L".jpg") || 
			!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 5, L".jpeg"))
		{
#ifdef SUPPORT_JPEG
			CJpegFile *pEBook = new CJpegFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".jpeg");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 4, L".tif") || 
			!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 5, L".tiff"))
		{
#ifdef SUPPORT_TIFF
			CTiffFile *pEBook = new CTiffFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".tiff");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else if(!_tcsicmp(FindData.cFileName + _tcslen(FindData.cFileName) - 4, L".bmp"))
		{
#ifdef SUPPORT_BMP
			CBmpFile *pEBook = new CBmpFile();
			pEBook->SetFileName(FindData.cFileName);
			pEBook->SetFileType(L".bmp");
			if(pEBook->Load(wszBuf))
				m_aEBooks.push_back(pEBook);
#endif
		}
		else
			continue;
	} while (FindNextFile(hFile, &FindData) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES) {
		wszTmpFolderPath[nPathLen] = '\0';
		goto error;
	}
	FindClose(hFile);

subdirs:
	/* Append * + \0 to wszTmpFolderPath. */
	wszTmpFolderPath[nSlashLen] = '*';
	wszTmpFolderPath[nSlashLen + 1] = '\0';
 
	/* Find first possible subdirectory. */
	hFile = FindFirstFileEx(wszTmpFolderPath,
		FindExInfoStandard, &FindData,
		FindExSearchLimitToDirectories, NULL, 0);
	if (hFile == INVALID_HANDLE_VALUE) {
		wszTmpFolderPath[nPathLen] = '\0';
		goto error;
	}
 
	/* Enter subdirectories. */ 
	do {
		const TCHAR *wszFileName = FindData.cFileName;
		const DWORD dwAttr = FindData.dwFileAttributes;
		INT nBufLen, nNameLen;

		/*
			* Skip '.' and '..', because they are links to
			* the current and parent directories, so they
			* are not subdirectories.
			*
			* Skip any file that is not a directory.
			*
			* Skip all reparse points, because they might
			* be symbolic links. They might form a cycle,
			* with a directory inside itself.
			*/
		if (_tcscmp(wszFileName, L".") == 0 ||
			_tcscmp(wszFileName, L"..") == 0 ||
			(dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0 ||
			(dwAttr & FILE_ATTRIBUTE_REPARSE_POINT))
			continue;
 
		/*
			* Allocate space for wszTmpFolderPath + backslash +
			*     wszFileName + backslash + wszPattern + \0.
			*/
		nNameLen = _tcslen(wszFileName);
		nBufLen = nSlashLen + nNameLen + nPatternLen + 2;
 
		/* Copy wszTmpFolderPath + backslash + wszFileName + \0. */
		_tcsncpy_s(wszBuf, MAX_PATH, wszTmpFolderPath, nSlashLen);
		_tcsncpy_s(wszBuf + nSlashLen, MAX_PATH - nSlashLen, wszFileName, nNameLen + 1);
 
		/* Push dir to list. Enter dir. */
		pSubFolder = new CDEFolder();
		m_aSubFolders.push_back(pSubFolder);
		if(!pSubFolder->Load(wszBuf))
			goto error;

	} while (FindNextFile(hFile, &FindData) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES) {
		wszTmpFolderPath[nPathLen] = '\0';
		goto error;
	}
	FindClose(hFile);

	return TRUE;
 
error:
	return FALSE;
}

JSONValue* CDEFolder::Serialize()
{
	JSONObject jsonRet;

	// Add folder path
	jsonRet[L"DIRECTORY"] = new JSONValue(m_wszFolderPath);

	// Add files
	JSONArray aBooks;
	list<CDEFile*>::iterator iEBook;
	for(iEBook = m_aEBooks.begin(); iEBook != m_aEBooks.end(); ++iEBook)
		aBooks.push_back(((COfficeFile*)*iEBook)->Serialize());
	jsonRet[L"FILES"] = new JSONValue(aBooks);

	// Add sub folders
	JSONArray aFolders;
	list<CDEFolder*>::iterator iEBookFolder;
	for(iEBookFolder = m_aSubFolders.begin(); iEBookFolder != m_aSubFolders.end(); ++iEBookFolder)
		aFolders.push_back(((CDEFolder*)*iEBookFolder)->Serialize());
	jsonRet[L"SUBDIRECTORIES"] = new JSONValue(aFolders);

	// Create a value
	JSONValue *value = new JSONValue(jsonRet);

	// return value
	return value;
}

UINT CDEFolder::GetEBookCount(TCHAR *wszType)
{
	UINT nCount = 0;

	list<CDEFile*>::iterator iEBook;
	for(iEBook = m_aEBooks.begin(); iEBook != m_aEBooks.end(); ++iEBook) {
		if(wszType == NULL) // all types
			nCount++;
		else if(!_tcscmp(((COfficeFile*)*iEBook)->GetFileType(), wszType))
			nCount++;
	}

	list<CDEFolder*>::iterator iEBookFolder;
	for(iEBookFolder = m_aSubFolders.begin(); iEBookFolder != m_aSubFolders.end(); ++iEBookFolder)
		nCount += ((CDEFolder*)*iEBookFolder)->GetEBookCount(wszType);

	return nCount;
}
