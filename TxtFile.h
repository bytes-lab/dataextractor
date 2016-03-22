#pragma once
#include "defile.h"

class CTxtFile :
	public CDEFile
{
public:
	CTxtFile(void);
	~CTxtFile(void);

private:
	TCHAR m_wszDateEdited[MAX_META_LEN];

public:
	virtual BOOL Load(TCHAR* wszFilePath);
	virtual JSONValue* Serialize(JSONObject * jsonMeta = NULL);
};
