#pragma once
#include "imagefile.h"
class CBmpFile :
	public CImageFile
{
public:
	CBmpFile(void);
	~CBmpFile(void);

public:
	virtual BOOL Load(TCHAR* wszFilePath);
	virtual JSONValue* Serialize(JSONObject * jsonMeta = NULL);
};
