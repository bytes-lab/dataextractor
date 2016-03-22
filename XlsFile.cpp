#include "StdAfx.h"

#include "XlsFile.h"
#include "Util.h"

#ifdef SUPPORT_XLS

CXlsFile::CXlsFile(void)
{
}

CXlsFile::~CXlsFile(void)
{
}

BOOL CXlsFile::Load(TCHAR* wszFilePath)
{
	IStorage *pStorage = NULL;
    IPropertySetStorage *pPropSetStg = NULL;
	IStream *pStream = NULL, *pSheetStream = NULL;
    HRESULT hr;
	
	// Open the document as an OLE compound document.
	hr = StgOpenStorage(wszFilePath, NULL,
    STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &pStorage);
	if(FAILED(hr)) {
		_tprintf( L"A file is locked by another program: [%s]\n", wszFilePath);
		return FALSE;
	}

#ifdef CONTENT_EXTRACTION
	hr = pStorage->OpenStream(L"Workbook", NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStream);
	if(FAILED(hr)) {
		pStorage->Release();
		_tprintf( L"A file is invalid or damaged: [%s]\n", wszFilePath);
		return FALSE;
	}
	struct  Record {
		USHORT usType;
		USHORT usSize;
	} recTmp = {0,0};

	BOOL bUnicode = FALSE;
	LARGE_INTEGER liTmpSeek;
	ULONG cbRead;

	UINT32 cstTotal, cstUnique;
	struct XLUnicodeRichExtendedString {
		USHORT usCch;
		BYTE usFlags;
		USHORT cRun;
		UINT32 cbExtRst;
	} StringConstants = {0,0,0,0};

	struct Cell {
		USHORT row;
		USHORT col;
		USHORT ifxe;
	} cell = {0,0,0};

	struct RK {
		USHORT rw;
		USHORT col;
		struct RKRec {
			USHORT ixfe;
			INT32 RkNumber;
		} rkRec;
	} rk = {0,0,{0,0}};

	struct MulRK {
		USHORT rw;
		USHORT colFirst;
		USHORT colLast;
		/* array of struct RKRec and colLast */
	} mrk = {0,0};

	struct RKRec {
		USHORT ixfe;
		ULONG	RkNumber;
	} rkRec = {0,0};

	struct BoundSheet {
		UINT32 lbPlyPos;
		BYTE hsState;
		BYTE dt;
		struct ShortXLUnicodeString {
			BYTE cch;
			BYTE fHighByte;
			//rgb string
		} string;
	} boundSheet;

	UINT32 nSstIndex = 0;
	DOUBLE dNumber = 0;
	INT32 nNumber = 0;

	// reading atom records
	while (1)
	{
		if(pStream->Read(&recTmp.usType, 2, &cbRead) != S_OK)
			break;
		if(pStream->Read(&recTmp.usSize, 2, &cbRead) != S_OK)
			break;
		if(!cbRead)
			break;
        
		switch(recTmp.usType)
		{
		case 133:
			{
			pStream->Read(&boundSheet.lbPlyPos, 4, &cbRead);
			pStream->Read(&boundSheet.hsState, 1, &cbRead);
			pStream->Read(&boundSheet.dt, 1, &cbRead);
			pStream->Read(&boundSheet.string.cch, 1, &cbRead);
			pStream->Read(&boundSheet.string.fHighByte, 1, &cbRead);

			wstring wsTmp;
			if(boundSheet.dt == 0x00) {// worksheet type
				if(boundSheet.string.fHighByte == 0x1) {
					WCHAR wChar;
					for(UINT j=0; j < boundSheet.string.cch; j++) {
						pStream->Read(&wChar, 2, NULL);
						wsTmp += (WCHAR)wChar;
					}
				} else {
					CHAR cChar;
					for(UINT j=0; j < boundSheet.string.cch; j++) {
						pStream->Read(&cChar, 1, NULL);
						wsTmp += (WCHAR)cChar;
					}
				}
			}

			m_aSheetNames.push_back(wsTmp);
			m_aSheetOffsets.push_back(boundSheet.lbPlyPos);
			break;
			}

		case 252: //SST
			{
			pStream->Read(&cstTotal, 4, NULL);
			pStream->Read(&cstUnique, 4, NULL);

			for (UINT i = 0; i < cstUnique; i++) {

				pStream->Read(&StringConstants.usCch, 2, NULL);
				pStream->Read(&StringConstants.usFlags, 1, NULL);

				if(StringConstants.usFlags & 0x8)
					pStream->Read(&StringConstants.cRun, 2, NULL);
				if(StringConstants.usFlags & 0x4)
					pStream->Read(&StringConstants.cbExtRst, 4, NULL);

				if(StringConstants.usFlags & 0x01) { //added high byte
					wstring wsTmp;
					WCHAR wChar;
					for(UINT j=0; j < StringConstants.usCch; j++) {
						pStream->Read(&wChar, 2, NULL);
						wsTmp += (WCHAR)wChar;
					}
					m_aSharedStrings.push_back(wsTmp);
				} else {
					wstring wsTmp;
					CHAR cChar;
					for(UINT j=0; j < StringConstants.usCch; j++) {
						pStream->Read(&cChar, 1, NULL);
						wsTmp += (WCHAR)cChar;
					}
					m_aSharedStrings.push_back(wsTmp);
				}
				// skip rgRun and ExtRst
				if(StringConstants.usFlags & 0x8) {
					liTmpSeek.QuadPart = StringConstants.cRun * 4;
					pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
				}
				if(StringConstants.usFlags & 0x4) {
					liTmpSeek.QuadPart = StringConstants.cbExtRst;
					pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
				}
			}
			break;
			}
		default:
			// seek as records len
			liTmpSeek.QuadPart = recTmp.usSize;
			pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
		}
	}

	// Read each sheet contents from sheet substreams
	for(UINT i = 0; i < m_aSheetNames.size(); i++) {

		// Seek to subsheet stream
		liTmpSeek.QuadPart = m_aSheetOffsets.at(i);
		pStream->Seek(liTmpSeek, STREAM_SEEK_SET, NULL);

		m_wszContent += m_aSheetNames.at(i);
		m_wszContent += L": ";

		BOOL endFlag = FALSE;
		while (1)
		{
			if(pStream->Read(&recTmp.usType, 2, &cbRead) != S_OK)
				break;
			if(pStream->Read(&recTmp.usSize, 2, &cbRead) != S_OK)
				break;
			if(!cbRead)
				break;
			switch(recTmp.usType) {
			case 638: //Rk
				pStream->Read(&rk.rw, 2, NULL);
				pStream->Read(&rk.col, 2, NULL);
				pStream->Read(&rk.rkRec.ixfe, 2, NULL);
				pStream->Read(&rk.rkRec.RkNumber, 4, NULL);

				if(rkRec.RkNumber & 0x00000002) {// signed integer or not
					nNumber = rkRec.RkNumber & 0xFFFFFFFC;

					if(rkRec.RkNumber & 0x80000000) // divided by 100 or not
						nNumber = nNumber * 100;

					m_wszContent += to_wstring((LONGLONG)nNumber);

				} else {

					*(DWORD*)&dNumber = 0;
					*(DWORD*)(((BYTE*)&dNumber) + 4)  = (rkRec.RkNumber & 0xFFFFFFFC);

					if(rkRec.RkNumber & 0x80000000) // divided by 100 or not
						dNumber = dNumber * 100;

					m_wszContent += to_wstring((LONGLONG)dNumber);
				}
				m_wszContent += L" ";

				break;
			case 189: //MulRk
				pStream->Read(&mrk.rw, 2, NULL);
				pStream->Read(&mrk.colFirst, 2, NULL);
				for (INT i = 0; i < ((recTmp.usSize - 2 - 2 - 2) / 6); i++) {
					pStream->Read(&rkRec.ixfe, 2, NULL);
					pStream->Read(&rkRec.RkNumber, 4, NULL);

					if(rkRec.RkNumber & 0x00000002) {// signed integer or not
						nNumber = rkRec.RkNumber & 0xFFFFFFFC;

						if(rkRec.RkNumber & 0x80000000) // divided by 100 or not
							nNumber = nNumber * 100;

						m_wszContent += to_wstring((LONGLONG)nNumber);

					} else {

						*(DWORD*)&dNumber = 0;
						*(DWORD*)(((BYTE*)&dNumber) + 4)  = (rkRec.RkNumber & 0xFFFFFFFC);

						if(rkRec.RkNumber & 0x80000000) // divided by 100 or not
							dNumber = dNumber * 100;

						m_wszContent += to_wstring((LONGLONG)dNumber);
					}
					m_wszContent += L" ";
				}
				pStream->Read(&mrk.colLast, 2, NULL);

				break;
	/*		case 215: //Number
				// seek as records len
				liTmpSeek.QuadPart = recTmp.usSize;
				pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);
				break;
			case 516: //Label
				pStream->Read(&cell, 6, NULL);
				pStream->Read(&UnicodeString.usCch, 2, NULL);
				pStream->Read(&UnicodeString.usFlags, 1, NULL);

				if(UnicodeString.usFlags & 0x80) { //added high byte
					WCHAR cChar;
					for(UINT j=0; j < UnicodeString.usCch; j++) {
						pStream->Read(&cChar, 2, NULL);
						m_wszContent += (WCHAR)cChar;
					}
				} else {
					CHAR cChar;
					for(UINT j=0; j < UnicodeString.usCch; j++) {
						pStream->Read(&cChar, 1, NULL);
						m_wszContent += (WCHAR)cChar;
					}
				}
				break;*/
			case 253: //LabelSst
				pStream->Read(&cell, 6, NULL);
				pStream->Read(&nSstIndex, 4, NULL);
				m_wszContent += m_aSharedStrings.at(nSstIndex);
				m_wszContent += L' ';
				break;
			default:

				// seek as records len
				liTmpSeek.QuadPart = recTmp.usSize;
				pStream->Seek(liTmpSeek, STREAM_SEEK_CUR, NULL);

				if(10 == recTmp.usType) // EOF end of substream
					endFlag = TRUE;

			}
			if(endFlag)
				break;
		}
	}
	pStream->Release();

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
//		{L"Status",			  PIDMSI_STATUS},
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