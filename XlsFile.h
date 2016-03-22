#pragma once
#include "officefile.h"

class CXlsFile :
	public COfficeFile
{
public:
	CXlsFile(void);
	~CXlsFile(void);

private:
	std::vector<std::wstring> m_aSharedStrings;
	std::vector<std::wstring> m_aSheetNames;
	std::vector<UINT> m_aSheetOffsets;

public:
	virtual BOOL Load(TCHAR* wszFilePath);
};