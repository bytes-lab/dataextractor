#include "StdAfx.h"

#include "PptFile.h"
#include "Util.h"

#ifdef SUPPORT_PPT

CPptFile::CPptFile(void)
{
}

CPptFile::~CPptFile(void)
{
}

BOOL CPptFile::ReadMeta(TCHAR* wszFilePath)
{
	IStorage *pStorage = NULL;
    IPropertySetStorage *pPropSetStg = NULL;
    HRESULT hr;
	
	// Open the document as an OLE compound document.
	hr = ::StgOpenStorage(wszFilePath, NULL,
    STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &pStorage);
	if(FAILED(hr)) {
		_tprintf( L"A file is locked by another program: [%s]\n", wszFilePath);
		return FALSE;
	}
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
		{L"Thumpnail",        PIDSI_THUMBNAIL},
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
		case PIDDSI_SLIDECOUNT:
			DumpPropVariant(pDocPropVar + i, m_wszPageCount, MAX_META_LEN);
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

/* ---ppt file format ----*/
typedef struct PptRecordHeader {
	USHORT usFlags;
	USHORT usType;
	UINT32 usLen;
} PPT_RECORD_HEADER;
/*------------------------*/

// Seek to a record starting location and return length of a record.
// If not found, return 0
UINT PptSeekToContainer(IStream *pStream, USHORT usType, USHORT usFlag)
{
	PPT_RECORD_HEADER tmpRecord = {0,0,0};
	BOOL bFound = FALSE;
	LARGE_INTEGER liTmpSeek;
	ULONG cbRead = 0;

	// reading records
	while (TRUE)
	{
		BYTE nType = 0;
		if(pStream->Read(&tmpRecord.usFlags, 2, &cbRead) != S_OK)
			break;
		if(pStream->Read(&tmpRecord.usType, 2, &cbRead) != S_OK)
			break;
		if(pStream->Read(&tmpRecord.usLen, 4, &cbRead) != S_OK)
			break;
		if(!cbRead)
			break;
        
		// found
		if(tmpRecord.usType == usType){// && (!usFlag || tmpRecord.usFlags == usFlag)) {
			bFound = TRUE;
			break;
		}

		// continue when container records
		if((tmpRecord.usFlags & 0xF) == 0xF)
			continue;

		// seek to as records len
		liTmpSeek.QuadPart = tmpRecord.usLen;
		pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
	}

	if(bFound) {
		return tmpRecord.usLen;
	}

	return 0;
}

VOID PptReadTextAtoms(IStream *pStream, std::vector<UINT> aNoteIds, wstring *pwsOutSlides, wstring *pwsOutNotes)
{
	PPT_RECORD_HEADER tmpRecord = {0,0,0};
	BOOL bFound = FALSE;
	LARGE_INTEGER liTmpSeek;
	ULONG cbRead = 0;
	BOOL bNoteFlag = FALSE;

	while (TRUE) {

		if(pStream->Read(&tmpRecord.usFlags, 2, &cbRead) != S_OK)
			break;
		if(pStream->Read(&tmpRecord.usType, 2, &cbRead) != S_OK)
			break;
		if(pStream->Read(&tmpRecord.usLen, 4, &cbRead) != S_OK)
			break;
		if(!cbRead)
			break;
        
		switch(tmpRecord.usType)
		{
		case 0x03F3://RT_SlidePersistAtom
			{
			UINT32 nPersistIdRef = 0;
			pStream->Read(&nPersistIdRef, 4, NULL);
			bNoteFlag = FALSE;
			// If exist in Notes list
			for (UINT i = 0; i < aNoteIds.size(); i++)
				if(nPersistIdRef == aNoteIds.at(i))
					bNoteFlag = TRUE;
			liTmpSeek.QuadPart = 16;
			pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
			break;
			}
		case 0x0F9F:
			cbRead++;
			// seek as records len
			liTmpSeek.QuadPart = tmpRecord.usLen;
			pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
			break;
		case 0x0FA0://RT_TextCharsAtom
			{
			WCHAR *aChars = new WCHAR[tmpRecord.usLen+1];
			ZeroMemory(aChars, (tmpRecord.usLen+1)*sizeof(WCHAR));
			pStream->Read(aChars, tmpRecord.usLen, NULL);
			for(UINT i=0; i < tmpRecord.usLen/2; i++) {
				if(bNoteFlag)
					*pwsOutNotes += (WCHAR)aChars[i];
				else
					*pwsOutSlides += (WCHAR)aChars[i];
			}
			delete aChars;
			break;
			}
		case 0x0FA8://RT_TextBytesAtom
			{
			BYTE *aChars = new BYTE[tmpRecord.usLen+1];
			ZeroMemory(aChars, tmpRecord.usLen+1);
			pStream->Read(aChars, tmpRecord.usLen, NULL);
			for(UINT i=0; i < tmpRecord.usLen; i++) {
				if(bNoteFlag)
					*pwsOutNotes += (WCHAR)aChars[i];
				else
					*pwsOutSlides += (WCHAR)aChars[i];
			}
			delete aChars;
			break;
			}

		case 0x03F8: // RT_MainMaster
			liTmpSeek.QuadPart = tmpRecord.usLen;
			pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
			break;

		case 0x0FF0: // RT_SlideListWithText
			if(tmpRecord.usFlags == 0x1F || tmpRecord.usFlags == 0x2F || tmpRecord.usFlags == 0x0F) {
				// seek as records len
				liTmpSeek.QuadPart = tmpRecord.usLen;
				pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
			}
			break;

		default:
			// continue when container records
			if((tmpRecord.usFlags & 0xF) == 0xF)
				break;

			// seek as records len
			liTmpSeek.QuadPart = tmpRecord.usLen;
			pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
			break;
		}
	}
}

VOID PptReadNotesIds(IStream *pStream, std::vector<UINT> *paNoteIds)
{
	PPT_RECORD_HEADER tmpRecord = {0,0,0};
	BOOL bFound = FALSE;
	LARGE_INTEGER liTmpSeek;
	ULONG cbRead = 0;
	ULONG nReadLen = 0;

	while (TRUE) {

		UINT nNotesContainerLen = 0;
		nNotesContainerLen = PptSeekToContainer(pStream, 0x03EE, 0); //SlideContainer

		// SlideAtom
		if(pStream->Read(&tmpRecord.usFlags, 2, &cbRead) != S_OK)
			break;
		if(pStream->Read(&tmpRecord.usType, 2, &cbRead) != S_OK)
			break;
		if(pStream->Read(&tmpRecord.usLen, 4, &cbRead) != S_OK)
			break;
		if(!cbRead)
			break;

		liTmpSeek.QuadPart = 16;
		pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);

		UINT32 nIdRef = 0;
		pStream->Read(&nIdRef, 4, NULL);
		if(nIdRef)
			paNoteIds->push_back(nIdRef);

		liTmpSeek.QuadPart = 4;
		pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);

		// seek as records len
		liTmpSeek.QuadPart = nNotesContainerLen - 32;/*size of SlideContainer*/
		pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
	}
}

BOOL CPptFile::ReadContents(TCHAR* wszFilePath)
{
	IStorage *pStorage = NULL;
	IStream *pStream = NULL;
 	LARGE_INTEGER liTmpSeek;
	HRESULT hr;
	
	// Open the document as an OLE compound document.
	hr = ::StgOpenStorage(wszFilePath, NULL,
    STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &pStorage);
	if(FAILED(hr)) {
		_tprintf( L"A file is locked by another program: [%s]\n", wszFilePath);
		return FALSE;
	}

	hr = pStorage->OpenStream(L"PowerPoint Document", NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStream);
	if(FAILED(hr)) {
		_tprintf( L"A file is invalid or damaged: [%s]\n", wszFilePath);
		pStorage->Release();
		return FALSE;
	}

	PptReadNotesIds(pStream, &m_aNotesIds);

	liTmpSeek.QuadPart = 0;
	pStream->Seek(liTmpSeek, STREAM_SEEK_SET, NULL);
	PptReadTextAtoms(pStream, m_aNotesIds, &m_wszContent, &m_wszSlideNotes);

	pStream->Release();
    pStorage->Release();

	return TRUE;
}

BOOL CPptFile::Load(TCHAR* wszFilePath)
{
	// Read Meta infomation
	if (!ReadMeta(wszFilePath))
		return FALSE;

#ifdef CONTENT_EXTRACTION
	// Read slide and notes contents
	if (!ReadContents(wszFilePath))
		return FALSE;
#endif

	return TRUE;
}

JSONValue* CPptFile::Serialize(JSONObject *jsonNone)
{
	JSONObject *jsonMeta = new JSONObject;
	(*jsonMeta)[L"SLIDE NOTES"] = new JSONValue(m_wszSlideNotes);

	return COfficeFile::Serialize(jsonMeta);
}
#endif