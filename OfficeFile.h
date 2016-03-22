#pragma once

#include "DEFile.h"

class COfficeFile : 
	public CDEFile
{
public:
	COfficeFile(void);
	~COfficeFile(void);

protected:
	// general meta information
	TCHAR m_wszAuthor[MAX_META_LEN];
	TCHAR m_wszTitle[MAX_META_LEN];
	TCHAR m_wszSubject[MAX_META_LEN];
	TCHAR m_wszKeywords[MAX_META_LEN];
	TCHAR m_wszStatus[MAX_META_LEN];
	TCHAR m_wszCompany[MAX_META_LEN];
	TCHAR m_wszApplication[MAX_META_LEN];
	TCHAR m_wszComments[MAX_META_LEN];
	TCHAR m_wszTemplateUsed[MAX_META_LEN];
	TCHAR m_wszRevisionNumber[MAX_META_LEN];
	TCHAR m_wszTotalEditingTime[MAX_META_LEN];
	TCHAR m_wszLastSaved[MAX_META_LEN];
	TCHAR m_wszLastEditedBy[MAX_META_LEN];
	TCHAR m_wszLastPrinted[MAX_META_LEN];
	TCHAR m_wszRecentHyperlinksList[MAX_META_LEN];
	TCHAR m_wszPageCount[MAX_META_LEN];
	TCHAR m_wszWordCount [MAX_META_LEN];
	TCHAR m_wszCharacterCount[MAX_META_LEN];
	TCHAR m_wszTrackChanges[MAX_META_LEN];
	TCHAR m_wszGUID[MAX_META_LEN];

public:
	virtual BOOL Load(TCHAR* wszPath){return TRUE;};
	virtual JSONValue* Serialize(JSONObject* jsonCustMeta = NULL);
};
