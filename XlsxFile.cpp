#include "StdAfx.h"

#include "XlsxFile.h"
#include "zip/unzip.h"

#ifdef SUPPORT_XLSX

CXlsxFile::CXlsxFile(void)
{
}

CXlsxFile::~CXlsxFile(void)
{
}

BOOL CXlsxFile::Load(TCHAR* wszFilePath)
{
	CMarkup *xmlContents, *xmlAppProp, *xmlCoreProp;
	HZIP hz;
	ZIPENTRY ze; 
	INT nIndex = -1; 
	CHAR *cBuf = NULL;

	// open docx file with zip lib.
	hz = OpenZip(wszFilePath, NULL);
	if(hz == NULL) {
		_tprintf( L"A file is locked by another program: [%s]\n", wszFilePath);
		return FALSE;
	}
	SetUnzipBaseDir(hz, L"\\"); // todo

	xmlContents = new CMarkup;
	xmlAppProp = new CMarkup;
	xmlCoreProp = new CMarkup;

	// open contents type document
	FindZipItem(hz, _T("[Content_Types].xml"), TRUE, &nIndex, &ze);
	cBuf = new CHAR[ze.unc_size];
	UnzipItem(hz, nIndex, L"DocExtractor.tmp");
	xmlContents->Load(L"\\DocExtractor.tmp");
	delete[] cBuf;
	DeleteFile(L"\\DocExtractor.tmp");

	// extract meta information
	if(xmlContents->FindElem(L"Types")) {
		xmlContents->IntoElem();
		while(xmlContents->FindElem(L"Override")) {
			if(xmlContents->GetAttrib(L"ContentType") == L"application/vnd.openxmlformats-package.core-properties+xml") {

				// load meta information from core properties.
				FindZipItem(hz, xmlContents->GetAttrib(L"PartName").c_str() + 1/*to remove first slash*/, TRUE, &nIndex, &ze);
				cBuf = new CHAR[ze.unc_size];
				UnzipItem(hz, nIndex, L"DocExtractor.tmp");
				xmlCoreProp->Load(L"\\DocExtractor.tmp");
				delete[] cBuf;
				DeleteFile(L"\\DocExtractor.tmp");
			}
			if(xmlContents->GetAttrib(L"ContentType") == L"application/vnd.openxmlformats-officedocument.extended-properties+xml") {

				// load meta information from app properties.
				FindZipItem(hz, xmlContents->GetAttrib(L"PartName").c_str() + 1/*to remove first slash*/, TRUE, &nIndex, &ze);
				cBuf = new CHAR[ze.unc_size];
				UnzipItem(hz, nIndex, L"DocExtractor.tmp");
				xmlAppProp->Load(L"\\DocExtractor.tmp");
				delete[] cBuf;
				DeleteFile(L"\\DocExtractor.tmp");
			}
		}
		xmlContents->ResetPos();
	}

	if(xmlCoreProp->FindElem(L"cp:coreProperties"))
		xmlCoreProp->IntoElem();
	if(xmlAppProp->FindElem(L"Properties"))
		xmlAppProp->IntoElem();

	if(xmlCoreProp->FindElem(L"dc:creator"))
		_tcscpy(m_wszAuthor, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();
	
	if(xmlCoreProp->FindElem(L"dc:title"))
		_tcscpy(m_wszTitle, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();

	if(xmlCoreProp->FindElem(L"dc:subject"))
		_tcscpy(m_wszSubject, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();

	if(xmlCoreProp->FindElem(L"cp:keywords"))
		_tcscpy(m_wszKeywords, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();

	if(xmlCoreProp->FindElem(L"cp:contentStatus"))
		_tcscpy(m_wszStatus, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();

	if(xmlAppProp->FindElem(L"Company"))
		_tcscpy(m_wszCompany, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();

	if(xmlAppProp->FindElem(L"Application"))
		_tcscpy(m_wszApplication, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();

	if(xmlCoreProp->FindElem(L"dc:description"))
		_tcscpy(m_wszComments, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();

	if(xmlAppProp->FindElem(L"Template"))
		_tcscpy(m_wszTemplateUsed, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();
	
	if(xmlCoreProp->FindElem(L"cp:revision"))
		_tcscpy(m_wszRevisionNumber, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();
	
	if(xmlAppProp->FindElem(L"TotalTime"))
		_tcscpy(m_wszTotalEditingTime, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();

	if(xmlCoreProp->FindElem(L"dcterms:modified"))
		_tcscpy(m_wszLastSaved, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();
	
	if(xmlCoreProp->FindElem(L"cp:lastModifiedBy"))
		_tcscpy(m_wszLastEditedBy, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();
	
	if(xmlCoreProp->FindElem(L"cp:lastPrinted"))
		_tcscpy(m_wszLastPrinted, xmlCoreProp->GetData().c_str());
	xmlCoreProp->ResetMainPos();
	
	if(xmlAppProp->FindElem(L"HyperlinksChanged"))
		_tcscpy(m_wszRecentHyperlinksList, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();

	if(xmlAppProp->FindElem(L"Pages"))
		_tcscpy(m_wszPageCount, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();

	if(xmlAppProp->FindElem(L"Words"))
		_tcscpy(m_wszWordCount, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();
	
	if(xmlAppProp->FindElem(L"Characters"))
		_tcscpy(m_wszCharacterCount, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();

//	m_wszTrackChanges
//	m_wszSlideNotes
//	m_wszGUID

	delete xmlAppProp;
	delete xmlCoreProp;

#ifdef CONTENT_EXTRACTION
	// extract text contents
	if(xmlContents->FindElem(L"Types")) {
		xmlContents->IntoElem();
		while(xmlContents->FindElem(L"Override")) {
			if(xmlContents->GetAttrib(L"ContentType") == L"application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml") {

				CMarkup *xmlStrings = new CMarkup;
				FindZipItem(hz, xmlContents->GetAttrib(L"PartName").c_str() + 1/*to remove first slash*/, TRUE, &nIndex, &ze);
				cBuf = new CHAR[ze.unc_size];
				UnzipItem(hz, nIndex, L"DocExtractor.tmp");
				xmlStrings->Load(L"\\DocExtractor.tmp");
				delete[] cBuf;
				DeleteFile(L"\\DocExtractor.tmp");

				if(xmlStrings->FindElem(L"sst"))
					xmlStrings->IntoElem();

				// search in all depth nodes
				while(xmlStrings->FindElem(L"si")) {
					xmlStrings->IntoElem();
					xmlStrings->FindElem(L"t");
					m_aSharedStrings.push_back(xmlStrings->GetData());
					xmlStrings->OutOfElem();
				}
				delete xmlStrings;
			}
			
			if(xmlContents->GetAttrib(L"ContentType") == L"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml") {

				CMarkup *xmlStrings = new CMarkup;
				FindZipItem(hz, xmlContents->GetAttrib(L"PartName").c_str() + 1/*to remove first slash*/, TRUE, &nIndex, &ze);
				cBuf = new CHAR[ze.unc_size];
				UnzipItem(hz, nIndex, L"DocExtractor.tmp");
				xmlStrings->Load(L"\\DocExtractor.tmp");
				delete[] cBuf;
				DeleteFile(L"\\DocExtractor.tmp");

				if(xmlStrings->FindElem(L"workbook")) {
					xmlStrings->IntoElem();
					if(xmlStrings->FindElem(L"sheets"))
						xmlStrings->IntoElem();
				}

				// search in all depth nodes
				while(xmlStrings->FindElem(L"sheet")) {
					m_aSheetNames.push_back(xmlStrings->GetAttrib(L"name"));
				}
				delete xmlStrings;
			}

		}
		xmlContents->ResetPos();
	}

	// extract text contents
	if(xmlContents->FindElem(L"Types")) {
		xmlContents->IntoElem();
		while(xmlContents->FindElem(L"Override")) {
			if(xmlContents->GetAttrib(L"ContentType") == L"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml") {

				CMarkup *xmlStrings = new CMarkup;
				FindZipItem(hz, xmlContents->GetAttrib(L"PartName").c_str() + 1/*to remove first slash*/, TRUE, &nIndex, &ze);
				cBuf = new CHAR[ze.unc_size];
				UnzipItem(hz, nIndex, L"DocExtractor.tmp");
				xmlStrings->Load(L"\\DocExtractor.tmp");
				delete[] cBuf;
				DeleteFile(L"\\DocExtractor.tmp");

				// attach sheet name:
				TCHAR wszIndex[2];
				wszIndex[1] = L'\0';
				wszIndex[0] = _tcsstr(xmlContents->GetAttrib(L"PartName").c_str(), L"/sheet")[6];
				m_wszContent += m_aSheetNames.at(stoi(wszIndex)-1);
				m_wszContent += L": ";

				if(xmlStrings->FindElem(L"worksheet"))
					xmlStrings->IntoElem();

				// search in all depth nodes
				while(xmlStrings->FindElem()) {
					ReadNodeContent(xmlStrings);
					if(!xmlStrings->IntoElem())
						continue;
					while(xmlStrings->FindElem()) {
						ReadNodeContent(xmlStrings);
						if(!xmlStrings->IntoElem())
							continue;
						while(xmlStrings->FindElem()) {
							ReadNodeContent(xmlStrings);
							if(!xmlStrings->IntoElem())
								continue;
							while(xmlStrings->FindElem()) {
								ReadNodeContent(xmlStrings);
								if(!xmlStrings->IntoElem())
									continue;
								while(xmlStrings->FindElem()) {
									ReadNodeContent(xmlStrings);
								}
								xmlStrings->OutOfElem();
							}
							xmlStrings->OutOfElem();
						}
						xmlStrings->OutOfElem();
					}
					xmlStrings->OutOfElem();
				}
				delete xmlStrings;
			}
		}
		xmlContents->ResetPos();
	}
#endif

	delete xmlContents;

	return TRUE;
}

void CXlsxFile::ReadNodeContent(CMarkup *xmlNode)
{
	wstring strTagName;
	strTagName = xmlNode->GetTagName();
	
	if(strTagName == L"row") // new row
		m_wszContent += '\n';

	if(strTagName == L"c") { // cell
		if((xmlNode->GetAttrib(L"t") == L"s")) {
			xmlNode->IntoElem();
			xmlNode->FindElem(L"v");
			m_wszContent += m_aSharedStrings.at(stoi(xmlNode->GetData()));
			m_wszContent += L" ";
		} else {
			xmlNode->IntoElem();
			xmlNode->FindElem(L"v");
			m_wszContent += xmlNode->GetData();
			m_wszContent += L" ";
		}
		xmlNode->OutOfElem();
	}
}
#endif