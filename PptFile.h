#pragma once

#include "officefile.h"

class CPptFile :
	public COfficeFile
{
public:
	CPptFile(void);
	~CPptFile(void);

private:
	std::vector<UINT> m_aSlideIds;
	std::vector<UINT> m_aNotesIds;

private:
	BOOL ReadMeta(TCHAR* wszFilePath);
	BOOL ReadContents(TCHAR* wszFilePath);

	wstring m_wszSlideNotes;

public:
	virtual BOOL Load(TCHAR* wszFilePath);
	virtual JSONValue* Serialize(JSONObject * jsonMeta = NULL);
};
