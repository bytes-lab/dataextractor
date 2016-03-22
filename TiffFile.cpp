#include "StdAfx.h"

#include "TiffFile.h"

#ifdef SUPPORT_TIFF

CTiffFile::CTiffFile(void)
{
}

CTiffFile::~CTiffFile(void)
{
}

BOOL CTiffFile::Load(TCHAR* wszFilePath)
{
    TiffImage tiffImage(BasicIo::AutoPtr(new FileIo(wszFilePath)), false);
    tiffImage.readMetadata();

    ExifData& exifData = tiffImage.exifData();
    if (!exifData.empty()) {
		DumpExifData(exifData);
    }

	// extract metas from EXIF
	DumpPhototimeOfExifData(exifData);
	DumpDescriptionOfExifData(exifData);
	DumpCameraSettingsOfExifData(exifData);
	DumpGeoTagOfExifData(exifData);
	/*
	XmpData& xmlData = tiffImage.xmpData();
    if (!xmlData.empty()) {
		DumpXmpData(xmlData);
    }
	*/
	return TRUE;
}

JSONValue* CTiffFile::Serialize(JSONObject *jsonNone)
{
	return CImageFile::Serialize();
}
#endif