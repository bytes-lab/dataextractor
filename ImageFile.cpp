#include "StdAfx.h"

#include "ImageFile.h"

#if defined SUPPORT_BMP || defined SUPPORT_JPEG || defined SUPPORT_TIFF

CImageFile::CImageFile(void)
{
	ZeroMemory(m_wszTakenTime, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszDescription, MAX_META_LEN * sizeof(TCHAR));

	m_wszXmp.clear();
	m_wszEXIF.clear();
	m_wszCameraSettings.clear();
	m_wszGeoTag.clear();
}

CImageFile::~CImageFile(void)
{
}

VOID CImageFile::DumpXmpData(const XmpData& xmpData)
{
	TCHAR wszBuf[1024];

	Exiv2::XmpData::const_iterator end = xmpData.end();
    for (Exiv2::XmpData::const_iterator i = xmpData.begin(); i != end; ++i) {
		INT nLen = 0;
		nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
		if( MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), strlen(i->key().c_str())+1, wszBuf, nLen))
			m_wszXmp += wszBuf;
		m_wszXmp += L": ";

		nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
		if( MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->value().toString().c_str(), strlen(i->value().toString().c_str())+1, wszBuf, nLen))
			m_wszXmp += wszBuf;
		m_wszXmp += L"\n";
    }
}

VOID CImageFile::DumpExifData(const ExifData& exifData)
{
	TCHAR wszBuf[1024];
    Exiv2::ExifData::const_iterator end = exifData.end();
    for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
		INT nLen = 0;
		nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
		if( MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), strlen(i->key().c_str())+1, wszBuf, nLen))
			m_wszEXIF += wszBuf;
		m_wszEXIF += L": ";

		nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
		if( MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->value().toString().c_str(), strlen(i->value().toString().c_str())+1, wszBuf, nLen))
			m_wszEXIF += wszBuf;
		m_wszEXIF += L"\n";
    }
}

VOID CImageFile::DumpPhototimeOfExifData(const ExifData& exifData)
{
	TCHAR wszBuf[1024];

	Exiv2::ExifData::const_iterator end = exifData.end();
    for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
		INT nLen = 0;
		nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
		if( MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), strlen(i->key().c_str())+1, wszBuf, nLen)) {
			if(!_tcscmp(wszBuf, L"Exif.Photo.DateTimeOriginal")) {
				nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
				MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->value().toString().c_str(), strlen(i->value().toString().c_str())+1, m_wszTakenTime, nLen);
			}
		}
    }
}

VOID CImageFile::DumpCameraSettingsOfExifData(const ExifData& exifData)
{
	TCHAR wszBuf[1024];

	Exiv2::ExifData::const_iterator end = exifData.end();
    for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
		INT nLen = 0;
		nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
		if( MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), strlen(i->key().c_str())+1, wszBuf, nLen)) {
			if(!_tcscmp(wszBuf, L"Exif.Image.Make")) {
				m_wszCameraSettings += L"Camera maker: ";
			} else if(!_tcscmp(wszBuf, L"Exif.Image.Model")) {
				m_wszCameraSettings += L"Camera model: ";
			} else if(!_tcscmp(wszBuf, L"Exif.Photo.FNumber")) {
				m_wszCameraSettings += L"F-stop: ";
			} else if(!_tcscmp(wszBuf, L"Exif.Photo.ExposureTime")) {
				m_wszCameraSettings += L"Exposure time: ";
			} else if(!_tcscmp(wszBuf, L"Exif.Photo.ISOSpeedRatings")) {
				m_wszCameraSettings += L"ISO speed: ";
			} else if(!_tcscmp(wszBuf, L"Exif.Photo.ExposureBiasValue")) {
				m_wszCameraSettings += L"Exposure bias: ";
			} else if(!_tcscmp(wszBuf, L"Exif.Photo.FocalLength")) {
				m_wszCameraSettings += L"Focal length: ";
			} else if(!_tcscmp(wszBuf, L"Exif.Photo.MaxApertureValue")) {
				m_wszCameraSettings += L"Max aperture: ";
			} else if(!_tcscmp(wszBuf, L"Exif.Photo.MeteringMode")) {
				m_wszCameraSettings += L"Metering mode: ";
			} else if(!_tcscmp(wszBuf, L"??")) {
				m_wszCameraSettings += L"Subject distance: ";
			} else if(!_tcscmp(wszBuf, L"Exif.Photo.Flash")) {
				m_wszCameraSettings += L"Flash mode: ";
			} else if(!_tcscmp(wszBuf, L"??")) {
				m_wszCameraSettings += L"Flash energy: ";
			} else if(!_tcscmp(wszBuf, L"??")) {
				m_wszCameraSettings += L"35mm focal length: ";
			} else
				continue;

			nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
			MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->value().toString().c_str(), strlen(i->value().toString().c_str())+1, wszBuf, nLen);
			wszBuf[nLen] = L'\0';
			m_wszCameraSettings += wszBuf;
			m_wszCameraSettings += L"\n";

		}
    }
}

VOID CImageFile::DumpGeoTagOfExifData(const ExifData& exifData)
{
	TCHAR wszBuf[1024];

	Exiv2::ExifData::const_iterator end = exifData.end();
    for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
		INT nLen = 0;
		nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
		if( MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), strlen(i->key().c_str())+1, wszBuf, nLen)) {
			if(!_tcscmp(wszBuf, L"Exif.GPSInfo.GPSVersionID")) {
				m_wszGeoTag += L"GPSVersionID: ";
			} else if(!_tcscmp(wszBuf, L"Exif.GPSInfo.GPSLatitudeRef")) {
				m_wszGeoTag += L"GPSLatitudeRef: ";
			} else if(!_tcscmp(wszBuf, L"Exif.GPSInfo.GPSLatitude")) {
				m_wszGeoTag += L"GPSLatitude: ";
			} else if(!_tcscmp(wszBuf, L"Exif.GPSInfo.GPSLongitudeRef")) {
				m_wszGeoTag += L"GPSLongitudeRef: ";
			} else if(!_tcscmp(wszBuf, L"Exif.GPSInfo.GPSLongitude")) {
				m_wszGeoTag += L"GPSLongitude: ";
			} else if(!_tcscmp(wszBuf, L"Exif.GPSInfo.GPSTimeStamp")) {
				m_wszGeoTag += L"GPSTimeStamp: ";
			} else if(!_tcscmp(wszBuf, L"Exif.GPSInfo.GPSMapDatum")) {
				m_wszGeoTag += L"GPSMapDatum: ";
			} else
				continue;

			nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
			MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->value().toString().c_str(), strlen(i->value().toString().c_str())+1, wszBuf, nLen);
			wszBuf[nLen] = L'\0';
			m_wszGeoTag += wszBuf;
			m_wszGeoTag += L"\n";
		}
    }
}

VOID CImageFile::DumpDescriptionOfExifData(const ExifData& exifData)
{
	TCHAR wszBuf[1024];

	Exiv2::ExifData::const_iterator end = exifData.end();
    for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
		INT nLen = 0;
		nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
		if( MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), strlen(i->key().c_str())+1, wszBuf, nLen)) {
			if(!_tcscmp(wszBuf, L"Exif.Photo.UserComment")) {
				nLen = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->key().c_str(), -1, NULL, NULL);
				MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, i->value().toString().c_str(), strlen(i->value().toString().c_str())+1, m_wszDescription, nLen);
			}
		}
    }
}

JSONValue* CImageFile::Serialize(JSONObject* jsonCustMeta)
{
	JSONObject *jsonMeta;

	if(jsonCustMeta != NULL)
		jsonMeta = jsonCustMeta;
	else
		jsonMeta = new JSONObject;

	(*jsonMeta)[L"POTO TAKEN"] = new JSONValue(m_wszTakenTime);
	(*jsonMeta)[L"EXIF"] = new JSONValue(m_wszEXIF.c_str());
	(*jsonMeta)[L"CAMERA SETTINGS"] = new JSONValue(m_wszCameraSettings);
	(*jsonMeta)[L"GEO TAG"] = new JSONValue(m_wszGeoTag);
	(*jsonMeta)[L"DESCRIPTION"] = new JSONValue(m_wszDescription);

	return CDEFile::Serialize(jsonMeta);
}
#endif