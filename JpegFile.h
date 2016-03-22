#pragma once 
#include "imagefile.h"
class CJpegFile :
	public CImageFile
{
public:
	CJpegFile(void);
	~CJpegFile(void);

	virtual BOOL Load(TCHAR* wszFilePath);
	virtual JSONValue* Serialize(JSONObject * jsonMeta = NULL);
};
