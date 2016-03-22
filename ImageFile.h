#pragma once

#include <exiv2.hpp>
#include "defile.h"

using namespace Exiv2;

class CImageFile :
	public CDEFile
{
public:
	CImageFile(void);
	~CImageFile(void);

public:
	// general meta information
	TCHAR m_wszTakenTime[MAX_META_LEN];
	TCHAR m_wszDescription[MAX_META_LEN];

	std::wstring m_wszXmp;
	std::wstring m_wszEXIF;
	std::wstring m_wszCameraSettings;
	std::wstring m_wszGeoTag;

protected:
	VOID DumpExifData(const ExifData& exifData);
	VOID DumpPhototimeOfExifData(const ExifData& exifData);
	VOID DumpCameraSettingsOfExifData(const ExifData& exifData);
	VOID DumpGeoTagOfExifData(const ExifData& exifData);
	VOID DumpDescriptionOfExifData(const ExifData& exifData);
	VOID DumpXmpData(const XmpData& xmpData);

public:
	virtual BOOL Load(TCHAR* wszPath){return TRUE;};
	virtual JSONValue* Serialize(JSONObject* jsonCustMeta = NULL);
};
