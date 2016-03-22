#include "StdAfx.h"

#include "DocFile.h"
#include "Util.h"

#ifdef SUPPORT_DOC

CDocFile::CDocFile(void)
{
}

CDocFile::~CDocFile(void)
{
}

BOOL CDocFile::Load(TCHAR* wszFilePath)
{
	IStorage *pStorage = NULL;
    IPropertySetStorage *pPropSetStg = NULL;
	IStream *pStream = NULL, *pTableStream = NULL;
    HRESULT hr;
	
	// Open the document as an OLE compound document.
	hr = ::StgOpenStorage(wszFilePath, NULL,
    STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &pStorage);
	if(FAILED(hr)) {
		_tprintf( L"A file is locked by another program: [%s]\n", wszFilePath);
		return FALSE;
	}

#ifdef CONTENT_EXTRACTION

	hr = pStorage->OpenStream(L"WordDocument", NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStream);
	if(FAILED(hr)) {
		_tprintf( L"A file is invalid or damaged: [%s]\n", wszFilePath);
		pStorage->Release();
		return FALSE;
	}
	STATSTG stat;
	pStream->Stat(&stat, 0);
	UINT size = (UINT) stat.cbSize.QuadPart;
	BYTE *buffer = new BYTE[size];
	USHORT usFlag;
	INT	pdcOffset;
	UINT pdcLength;
	LARGE_INTEGER liTmpSeek;
	ULONG cbRead;
	liTmpSeek.QuadPart = 10;
	pStream->Seek(liTmpSeek, 0, NULL);
	pStream->Read(&usFlag, 2, &cbRead);
	liTmpSeek.QuadPart = 418;
	pStream->Seek(liTmpSeek, 0, NULL);
	pStream->Read(&pdcOffset, 4, &cbRead);
	pStream->Read(&pdcLength, 4, &cbRead);

	hr = pStorage->OpenStream((usFlag & (0x1 << 9)) ? L"1Table" : L"0Table", NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pTableStream);
	liTmpSeek.QuadPart = pdcOffset;
    pTableStream->Seek(liTmpSeek, 0, NULL);

	INT nCount = 0;
	INT *aOffsets = NULL;
	struct Desc {
		BYTE Flags;
		BYTE Fn;
		UINT32  Fc;
		SHORT Prm;
	} *aDescs = NULL;
	UINT nPointer = 0;

	while (nPointer < pdcLength)
	{
		BYTE nType = 0;
		pTableStream->Read(&nType, 1, &cbRead);
        
		switch(nType)
		{
		case 0:
			BYTE nTmp;
			pTableStream->Read(&nTmp, 1, &cbRead);
			nPointer++;
			break;

		case 1:
			{
			SHORT cbGrpprl;
			BYTE *grpprlData = NULL;
			pTableStream->Read(&cbGrpprl, 2, &cbRead);
			nPointer+=2;
			grpprlData = new BYTE[cbGrpprl];
			pTableStream->Read(grpprlData, cbGrpprl, &cbRead);
			nPointer+=cbGrpprl;
			delete grpprlData;
			break;
			}
		case 2:

			INT nTableLen = 0;
			pTableStream->Read(&nTableLen, 4, &cbRead);
			nPointer+=4;

			nCount = (nTableLen - 4) / (8 + 4) + 1;
			aOffsets = new INT[nCount];
			for(int i = 0; i < nCount; i++) {
				pTableStream->Read(&aOffsets[i], 4, &cbRead);
				nPointer +=4;
			}

			aDescs = new Desc[nCount-1];
			for(int i = 0; i < nCount-1; i++) {
				pTableStream->Read(&aDescs[i].Flags, 1, &cbRead);
				pTableStream->Read(&aDescs[i].Fn, 1, &cbRead);
				pTableStream->Read(&aDescs[i].Fc, 4, &cbRead);
				pTableStream->Read(&aDescs[i].Prm, 2, &cbRead);
				nPointer+=sizeof(struct Desc);
			}
			break;
		}
	}

	for (int i = 0; i < nCount - 1; i++)
	{
		UINT pieceStart = 0xffffffff;
		UINT pieceEnd = 0xffffffff;
		BOOL bUnicode = FALSE;

		bUnicode = (aDescs[i].Fc & (1 << 30)) == 0 ? 1 : 0;
		if (!bUnicode)
			pieceStart = (aDescs[i].Fc & ~(1 << 30)) / 2;
		else
			pieceStart = aDescs[i].Fc;

		UINT nLength = aOffsets[i + 1] - aOffsets[i];
		pieceEnd = pieceStart + nLength * (bUnicode ? 2 : 1);

		liTmpSeek.QuadPart = pieceStart;
		hr = pStream->Seek(liTmpSeek, 0, NULL);
		//m_wszContent += ReadString(documentReader, pieceEnd - pieceStart, isUnicode);
		if (nLength == 0)
			continue;

		for (UINT i = 0; i < nLength; i++)
		{
			if (!bUnicode)
			{
				BYTE ch = 0;
				pStream->Read(&ch, 1, NULL);
				m_wszContent += (CHAR)ch;
			}
			else
			{
				SHORT ch = 0;
				pStream->Read(&ch, 2, NULL);
				m_wszContent += (WCHAR)ch;
			}	
		}
	}
	pStream->Release();
	pTableStream->Release();

#endif

	// Obtain the IPropertySetStorage interface.
	hr = pStorage->QueryInterface(IID_IPropertySetStorage, (void **)&pPropSetStg);
	IPropertyStorage *pPropStg = NULL;

	struct pidsiStruct {
		TCHAR *name;
		LONG pid;
	} pidsiArr[] = {
		{L"Author",           PIDSI_AUTHOR},
		{L"Title",            PIDSI_TITLE},
		{L"Subject",          PIDSI_SUBJECT},
		{L"Keywords",         PIDSI_KEYWORDS},
		{L"Status",			  PIDMSI_STATUS},
		{L"AppName",          PIDSI_APPNAME}, 
		{L"Comments",         PIDSI_COMMENTS},
		{L"Template",         PIDSI_TEMPLATE},
		{L"Revision Number",  PIDSI_REVNUMBER},
		{L"Created",          PIDSI_CREATE_DTM},
		{L"Edit Time",        PIDSI_EDITTIME},
		{L"Last Saved",       PIDSI_LASTSAVE_DTM},
		{L"LastAuthor",       PIDSI_LASTAUTHOR},
		{L"Last printed",     PIDSI_LASTPRINTED},
		{L"Page Count",       PIDSI_PAGECOUNT},
		{L"Word Count",       PIDSI_WORDCOUNT},
		{L"Char Count",       PIDSI_CHARCOUNT},
		{L"Thumbnail",        PIDSI_THUMBNAIL},
		{L"Doc Security",     PIDSI_DOC_SECURITY},
		{0, 0}
	}, pidsdiArr[] = {
		{L"Company",		  PIDDSI_COMPANY},
		{L"Slide notes",      PIDDSI_SLIDECOUNT},
		{0, 0}
	};
	
	// Count elements in pidsiArr.
	INT nPidsi = 0, nPidsdi = 0;
	for(nPidsi=0; pidsiArr[nPidsi].name; nPidsi++);
	for(nPidsdi=0; pidsdiArr[nPidsdi].name; nPidsdi++);
	  
	// Initialize PROPSPEC for the properties you want.
	PROPSPEC *pPropSpec = new PROPSPEC [nPidsi];
	PROPVARIANT *pPropVar = new PROPVARIANT [nPidsi];
	PROPSPEC *pDocPropSpec = new PROPSPEC [nPidsdi];
	PROPVARIANT *pDocPropVar = new PROPVARIANT [nPidsdi];

	for(INT i=0; i<nPidsi; i++) {
		ZeroMemory(&pPropSpec[i], sizeof(PROPSPEC));
		pPropSpec[i].ulKind = PRSPEC_PROPID;
		pPropSpec[i].propid = pidsiArr[i].pid;
	}
	for(INT i=0; i<nPidsdi; i++) {
		ZeroMemory(&pDocPropSpec[i], sizeof(PROPSPEC));
		pDocPropSpec[i].ulKind = PRSPEC_PROPID;
		pDocPropSpec[i].propid = pidsdiArr[i].pid;
	}

	// Obtain meta infos from FMTID_SummaryInformation
	hr = pPropSetStg->Open(FMTID_SummaryInformation, STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStg);
	// Read properties.
	hr = pPropStg->ReadMultiple(nPidsi, pPropSpec, pPropVar);
	if(FAILED(hr)) {
         _tprintf(L"IPropertyStg::ReadMultiple() failed w/error %08lx",hr);
    }
	pPropStg->Release();
	pPropStg = NULL;

	// Obtain meta infos from FMTID_DocSummaryInformation
	hr = pPropSetStg->Open(FMTID_DocSummaryInformation, STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropStg);
	// Read properties.
	hr = pPropStg->ReadMultiple(nPidsdi, pDocPropSpec, pDocPropVar);
	if(FAILED(hr)) {
         _tprintf(L"IPropertyStg::ReadMultiple() failed w/error %08lx",hr);
    }
	pPropStg->Release();

	// Copy Meta fields out from FMTID_SummaryInformation
	for(int i = 0; i < nPidsi; i++) {
		switch(pidsiArr[i].pid) {
		case PIDSI_AUTHOR:
			DumpPropVariant(pPropVar + i, m_wszAuthor, MAX_META_LEN);
			break;
		case PIDSI_TITLE:
			DumpPropVariant(pPropVar + i, m_wszTitle, MAX_META_LEN);
			break;
		case PIDSI_SUBJECT:
			DumpPropVariant(pPropVar + i, m_wszSubject, MAX_META_LEN);
			break;
		case PIDSI_KEYWORDS:
			DumpPropVariant(pPropVar + i, m_wszKeywords, MAX_META_LEN);
			break;
		case PIDSI_APPNAME:
			DumpPropVariant(pPropVar + i, m_wszApplication, MAX_META_LEN);
			break;
		case PIDSI_COMMENTS:
			DumpPropVariant(pPropVar + i, m_wszComments, MAX_META_LEN);
			break;
		case PIDSI_TEMPLATE:
			DumpPropVariant(pPropVar + i, m_wszTemplateUsed, MAX_META_LEN);
			break;
		case PIDSI_REVNUMBER:
			DumpPropVariant(pPropVar + i, m_wszRevisionNumber, MAX_META_LEN);
			break;
		case PIDSI_EDITTIME:
			DumpPropVariant(pPropVar + i, m_wszTotalEditingTime, MAX_META_LEN);
			break;
		case PIDSI_LASTSAVE_DTM:
			DumpPropVariant(pPropVar + i, m_wszLastSaved, MAX_META_LEN);
			break;
		case PIDSI_LASTAUTHOR:
			DumpPropVariant(pPropVar + i, m_wszLastEditedBy, MAX_META_LEN);
			break;
		case PIDSI_LASTPRINTED:
			DumpPropVariant(pPropVar + i, m_wszLastPrinted, MAX_META_LEN);
			break;
		case PIDSI_PAGECOUNT:
			DumpPropVariant(pPropVar + i, m_wszPageCount, MAX_META_LEN);
			break;
		case PIDSI_WORDCOUNT:
			DumpPropVariant(pPropVar + i, m_wszWordCount, MAX_META_LEN);
			break;
		case PIDSI_CHARCOUNT:
			DumpPropVariant(pPropVar + i, m_wszCharacterCount, MAX_META_LEN);
			break;
		}
	}

	// Copy Meta fields out from FMTID_DocSummaryInformation
	for(int i = 0; i < nPidsdi; i++) {
		switch(pidsdiArr[i].pid) {
		case PIDDSI_COMPANY:
			DumpPropVariant(pDocPropVar + i, m_wszCompany, MAX_META_LEN);
			break;
		}
	}

	// De-allocate memory.
	delete [] pPropVar;
	delete [] pPropSpec;
	delete [] pDocPropVar;
	delete [] pDocPropSpec;

	// Release obtained interface.
	pPropSetStg->Release();
    pStorage->Release();

	return TRUE;
}
#endif