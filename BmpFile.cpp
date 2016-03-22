#include "StdAfx.h"

#include "BmpFile.h"

#ifdef SUPPORT_BMP

CBmpFile::CBmpFile(void)
{
}

CBmpFile::~CBmpFile(void)
{
}

BOOL CBmpFile::Load(TCHAR* wszFilePath)
{
    BmpImage bmpImage(BasicIo::AutoPtr(new FileIo(wszFilePath)));
    bmpImage.readMetadata();

    ExifData& exifData = bmpImage.exifData();
    if (!exifData.empty()) {
		DumpExifData(exifData);
    }

	// extract metas from EXIF
	DumpPhototimeOfExifData(exifData);
	DumpDescriptionOfExifData(exifData);
	DumpCameraSettingsOfExifData(exifData);
	DumpGeoTagOfExifData(exifData);

	return TRUE;
}

JSONValue* CBmpFile::Serialize(JSONObject *jsonNone)
{
	return CImageFile::Serialize();
}
#endif