#include "StdAfx.h"

#include "PptxFile.h"
#include "xml/Markup.h"
#include "zip/Unzip.h"

#ifdef SUPPORT_PPTX

CPptxFile::CPptxFile(void)
{
}

CPptxFile::~CPptxFile(void)
{
}

BOOL CPptxFile::Load(TCHAR* wszFilePath)
{
	CMarkup *xmlContents, *xmlAppProp, *xmlCoreProp;
	HZIP hz;
	ZIPENTRY ze; 
	INT nIndex = -1; 
	CHAR *cBuf = NULL;

	// open pptx file with zip lib.
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

	if(xmlAppProp->FindElem(L"Words"))
		_tcscpy(m_wszWordCount, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();
	
	if(xmlAppProp->FindElem(L"Characters"))
		_tcscpy(m_wszCharacterCount, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();

	if(xmlAppProp->FindElem(L"Slides"))
		_tcscpy(m_wszPageCount, xmlAppProp->GetData().c_str());
	xmlAppProp->ResetMainPos();
	
//	m_wszTrackChanges
//	m_wszGUID

	delete xmlAppProp;
	delete xmlCoreProp;

#ifdef CONTENT_EXTRACTION

	// extract text contents
	if(xmlContents->FindElem(L"Types")) {
		xmlContents->IntoElem();
		while(xmlContents->FindElem(L"Override")) {
			if(xmlContents->GetAttrib(L"ContentType") == L"application/vnd.openxmlformats-officedocument.presentationml.slide+xml") {

				// load slide texts.
				CMarkup *xmlSlide = new CMarkup;
				FindZipItem(hz, xmlContents->GetAttrib(L"PartName").c_str() + 1/*to remove first slash*/, TRUE, &nIndex, &ze);
				cBuf = new CHAR[ze.unc_size];
				UnzipItem(hz, nIndex, L"DocExtractor.tmp");
				xmlSlide->Load(L"\\DocExtractor.tmp");
				delete[] cBuf;
				DeleteFile(L"\\DocExtractor.tmp");

				// extracting text from a slide
				if(xmlSlide->FindElem(L"p:sld"))
					xmlSlide->IntoElem();
				if(xmlSlide->FindElem(L"p:cSld"))
					xmlSlide->IntoElem();

				// search in all depth nodes
				while(xmlSlide->FindElem()) {
					ReadNodeSlides(xmlSlide);
					if(!xmlSlide->IntoElem())
						continue;
					while(xmlSlide->FindElem()) {
						ReadNodeSlides(xmlSlide);
						if(!xmlSlide->IntoElem())
							continue;
						while(xmlSlide->FindElem()) {
							ReadNodeSlides(xmlSlide);
							if(!xmlSlide->IntoElem())
								continue;
							while(xmlSlide->FindElem()) {
								ReadNodeSlides(xmlSlide);
								if(!xmlSlide->IntoElem())
									continue;
								while(xmlSlide->FindElem()) {
									ReadNodeSlides(xmlSlide);
									if(!xmlSlide->IntoElem())
										continue;
									while(xmlSlide->FindElem()) {
										ReadNodeSlides(xmlSlide);
										if(!xmlSlide->IntoElem())
											continue;
										while(xmlSlide->FindElem()) {
											ReadNodeSlides(xmlSlide);
										}
										xmlSlide->OutOfElem();
									}
									xmlSlide->OutOfElem();
								}
								xmlSlide->OutOfElem();
							}
							xmlSlide->OutOfElem();
						}
						xmlSlide->OutOfElem();
					}
					xmlSlide->OutOfElem();
				}

				delete xmlSlide;
			}

			if(xmlContents->GetAttrib(L"ContentType") == L"application/vnd.openxmlformats-officedocument.presentationml.notesSlide+xml") {
				// load slide texts.
				CMarkup *xmlSlide = new CMarkup;
				FindZipItem(hz, xmlContents->GetAttrib(L"PartName").c_str() + 1/*to remove first slash*/, TRUE, &nIndex, &ze);
				cBuf = new CHAR[ze.unc_size];
				UnzipItem(hz, nIndex, L"DocExtractor.tmp");
				xmlSlide->Load(L"\\DocExtractor.tmp");
				delete[] cBuf;
				DeleteFile(L"\\DocExtractor.tmp");

				// extracting text from a slide
				if(xmlSlide->FindElem(L"p:notes"))
					xmlSlide->IntoElem();
				if(xmlSlide->FindElem(L"p:cSld"))
					xmlSlide->IntoElem();

				// search in all depth nodes
				while(xmlSlide->FindElem()) {
					ReadNodeNotes(xmlSlide);
					if(!xmlSlide->IntoElem())
						continue;
					while(xmlSlide->FindElem()) {
						ReadNodeNotes(xmlSlide);
						if(!xmlSlide->IntoElem())
							continue;
						while(xmlSlide->FindElem()) {
							ReadNodeNotes(xmlSlide);
							if(!xmlSlide->IntoElem())
								continue;
							while(xmlSlide->FindElem()) {
								ReadNodeNotes(xmlSlide);
								if(!xmlSlide->IntoElem())
									continue;
								while(xmlSlide->FindElem()) {
									ReadNodeNotes(xmlSlide);
									if(!xmlSlide->IntoElem())
										continue;
									while(xmlSlide->FindElem()) {
										ReadNodeNotes(xmlSlide);
										if(!xmlSlide->IntoElem())
											continue;
										while(xmlSlide->FindElem()) {
											ReadNodeNotes(xmlSlide);
										}
										xmlSlide->OutOfElem();
									}
									xmlSlide->OutOfElem();
								}
								xmlSlide->OutOfElem();
							}
							xmlSlide->OutOfElem();
						}
						xmlSlide->OutOfElem();
					}
					xmlSlide->OutOfElem();
				}

				delete xmlSlide;
			}
		}
		xmlContents->ResetPos();

	}
#endif

	delete xmlContents;
	return TRUE;
}

void CPptxFile::ReadNodeSlides(CMarkup *xmlNode)
{
	wstring strTagName;
	strTagName = xmlNode->GetTagName();
	
	if(strTagName == L"a:p") // paragraph
		m_wszContent += ' ';

	if(strTagName == L"a:tab") // tab
		m_wszContent += '\t';
	/*
	if(strTagName == L"a:r" || 
		strTagName == L"a:cr" || 
		strTagName == L"a:br") // breaks, new lines
		m_wszContent += '\n';
	*/
	if(strTagName == L"a:t") // text
		m_wszContent += xmlNode->GetData();
}

void CPptxFile::ReadNodeNotes(CMarkup *xmlNode)
{
	wstring strTagName;
	strTagName = xmlNode->GetTagName();
	
	if(strTagName == L"a:r")
		m_wszSlideNotes += '\n';

	if(strTagName == L"a:fld") // slide id
		xmlNode->RemoveElem();

	if(strTagName == L"a:t") // text
		m_wszSlideNotes += xmlNode->GetData();
}

JSONValue* CPptxFile::Serialize(JSONObject *jsonNone)
{
	JSONObject *jsonMeta = new JSONObject;
	(*jsonMeta)[L"SLIDE NOTES"] = new JSONValue(m_wszSlideNotes);

	return COfficeFile::Serialize(jsonMeta);
}
#endif