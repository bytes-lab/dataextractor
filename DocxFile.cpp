#include "StdAfx.h"

#include "DocxFile.h"
#include "xml/Markup.h"
#include "zip/Unzip.h"

#ifdef SUPPORT_DOCX

CDocxFile::CDocxFile(void)
{
}


CDocxFile::~CDocxFile(void)
{
}

BOOL CDocxFile::Load(TCHAR* wszFilePath)
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
			if(xmlContents->GetAttrib(L"ContentType") == L"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml") {

				CMarkup *xmlDocument = new CMarkup;
				FindZipItem(hz, xmlContents->GetAttrib(L"PartName").c_str() + 1/*to remove first slash*/, TRUE, &nIndex, &ze);
				cBuf = new CHAR[ze.unc_size];
				UnzipItem(hz, nIndex, L"DocExtractor.tmp");
				xmlDocument->Load(L"\\DocExtractor.tmp");
				delete[] cBuf;
				DeleteFile(L"\\DocExtractor.tmp");

				if(xmlDocument->FindElem(L"w:document"))
					xmlDocument->IntoElem();
				if(xmlDocument->FindElem(L"w:body"))
					xmlDocument->IntoElem();

				// search in all depth nodes
				SurfContent(xmlDocument);
				delete xmlDocument;
			}
		}
		xmlContents->ResetPos();
	}
#endif

	delete xmlContents;
	return TRUE;
}

void CDocxFile::SurfContent(CMarkup *xmlNode)
{
	while(xmlNode->FindElem()) {
		ReadNodeContent(xmlNode);
		if(xmlNode->IntoElem())
		{
			SurfContent(xmlNode);
			xmlNode->OutOfElem();
		}
	}
}

void CDocxFile::ReadNodeContent(CMarkup *xmlNode)
{
	wstring strTagName;
	strTagName = xmlNode->GetTagName();
	
	if(strTagName == L"w:p") // paragraph
		m_wszContent += L' ';
	else if(strTagName == L"w:tab") // tab
		m_wszContent += '\t';
	else if(strTagName == L"w:t") // text
		m_wszContent += xmlNode->GetData();

/*
	else if(strTagName == L"w:r" || 
		strTagName == L"w:cr" || 
		strTagName == L"w:br") // breaks, new lines
		m_wszContent += L'\n';
		*/
}
#endif