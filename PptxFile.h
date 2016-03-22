#pragma once

#include "officefile.h"
#include "xml/Markup.h"

class CPptxFile :
	public COfficeFile
{
public:
	CPptxFile(void);
	~CPptxFile(void);

private:
	void ReadNodeSlides(CMarkup *xmlNode);
	void ReadNodeNotes(CMarkup *xmlNode);

	wstring m_wszSlideNotes;

public:
	virtual BOOL Load(TCHAR* wszFilePath);
	virtual JSONValue* Serialize(JSONObject * jsonMeta = NULL);
};
