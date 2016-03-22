#include "StdAfx.h"

#include "JpegFile.h"

#ifdef SUPPORT_JPEG

CJpegFile::CJpegFile(void)
{
}

CJpegFile::~CJpegFile(void)
{
}

BOOL CJpegFile::Load(TCHAR* wszFilePath)
{
    JpegImage jpegImage(BasicIo::AutoPtr(new FileIo(wszFilePath)), false);
    jpegImage.readMetadata();

    ExifData& exifData = jpegImage.exifData();
    if (!exifData.empty()) {
		DumpExifData(exifData);
    }

	// extract metas from EXIF
	DumpPhototimeOfExifData(exifData);
	DumpDescriptionOfExifData(exifData);
	DumpCameraSettingsOfExifData(exifData);
	DumpGeoTagOfExifData(exifData);
	/*
	XmpData& xmlData = jpegImage.xmpData();
    if (!xmlData.empty()) {
		DumpXmpData(xmlData);
    }*/

	return TRUE;
}

JSONValue* CJpegFile::Serialize(JSONObject *jsonNone)
{
	return CImageFile::Serialize();
}
#endif