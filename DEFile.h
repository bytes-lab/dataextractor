#pragma once

#include "Json/JSON.h"

#define MAX_META_LEN 100

using namespace std;

class CDEFile
{
public:
	CDEFile(void);
	~CDEFile(void);

private:
	FILE		hFile;
	TCHAR		m_wszFileName[MAX_PATH];
	TCHAR		m_wszFileType[MAX_PATH];

public:
	wstring		m_wszContent;

public:
	VOID SetFileName(TCHAR* wszName);
	VOID SetFileType(TCHAR* wszType);
	TCHAR* GetFileType();

	virtual BOOL Load(TCHAR* wszPath){return TRUE;};
	virtual JSONValue* Serialize(JSONObject* jsonCustMeta);
};
