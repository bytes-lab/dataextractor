#pragma once
#include "imagefile.h"
class CTiffFile :
	public CImageFile
{
public:
	CTiffFile(void);
	~CTiffFile(void);

public:
	virtual BOOL Load(TCHAR* wszFilePath);
	virtual JSONValue* Serialize(JSONObject * jsonMeta = NULL);
};
