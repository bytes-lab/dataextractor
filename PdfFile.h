#pragma once

#include "defile.h"

#ifdef SUPPORT_PDF

extern "C" {
#include "fitz/fitz.h"
#include "pdf/mupdf.h"
}

class CPdfFile :
	public CDEFile
{
public:
	CPdfFile(void);
	~CPdfFile(void);

private:
    pdf_xref      * m_xrfDoc;
    fz_obj        * m_infDoc;
	VOID GetProperty(CHAR*, TCHAR*);

	TCHAR m_wszTitle[MAX_META_LEN];
	TCHAR m_wszSubject[MAX_META_LEN];
	TCHAR m_wszAuthor[MAX_META_LEN];
	TCHAR m_wszKeywords[MAX_META_LEN];
	TCHAR m_wszCreated[MAX_META_LEN];
	TCHAR m_wszDateEdited[MAX_META_LEN];

	wstring m_wszCommentAndReview;
	wstring m_wszAttachments;
	wstring m_wszXmp;

public:
	virtual BOOL Load(TCHAR* wszFilePath);
	virtual JSONValue* Serialize(JSONObject * jsonMeta = NULL);
};

#endif