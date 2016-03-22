#pragma once
#include "officefile.h"
#include "xml/Markup.h"

class CXlsxFile :
	public COfficeFile
{
public:
	CXlsxFile(void);
	~CXlsxFile(void);

private:
	std::vector<std::wstring> m_aSharedStrings;
	std::vector<std::wstring> m_aSheetNames;

private:
	void ReadNodeContent(CMarkup *xmlNode);

public:
	virtual BOOL Load(TCHAR* wszFilePath);
};

