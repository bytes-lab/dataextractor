#pragma once

#include "officefile.h"
#include "xml/Markup.h"

class CDocxFile :
	public COfficeFile
{
public:
	CDocxFile(void);
	~CDocxFile(void);

private:
	void ReadNodeContent(CMarkup *xmlNode);
	void SurfContent(CMarkup *xmlNode);

public:
	virtual BOOL Load(TCHAR* wszFilePath);
};

