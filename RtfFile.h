#pragma once
#include "defile.h"
class CRtfFile :
	public CDEFile
{
public:
	CRtfFile(void);
	~CRtfFile(void);

private:
	TCHAR m_wszDateEdited[MAX_META_LEN];

public:
	virtual BOOL Load(TCHAR* wszFilePath);
	virtual JSONValue* Serialize(JSONObject * jsonMeta = NULL);
};
