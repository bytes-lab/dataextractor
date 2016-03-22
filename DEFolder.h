#pragma once

#include <list>

#include "DEFile.h"
#include "Json/JSON.h"

using namespace std;

class CDEFolder
{
public:
	CDEFolder(void);
	~CDEFolder(void);

private:
	TCHAR				m_wszFolderPath[MAX_PATH];
	list<CDEFile*>		m_aEBooks;
	list<CDEFolder*>	m_aSubFolders;

public:
	BOOL Load(TCHAR* wszFolderPath);
	JSONValue* Serialize();

	UINT GetEBookCount(TCHAR *wszType);
};
