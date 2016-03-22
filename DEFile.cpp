#include "StdAfx.h"

#include "DEFile.h"

CDEFile::CDEFile(void)
{
	ZeroMemory(m_wszFileName, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszFileType, MAX_META_LEN * sizeof(TCHAR));

	m_wszContent.clear();
}

CDEFile::~CDEFile(void)
{
}

JSONValue* CDEFile::Serialize(JSONObject* jsonCustMeta)
{
	JSONObject jsonRet;

	jsonRet[L"FILENAME"] = new JSONValue(m_wszFileName);
	jsonRet[L"FILETYPE"] = new JSONValue(m_wszFileType);

	jsonRet[L"META-INFORMATION"] = new JSONValue(*jsonCustMeta);

#ifdef CONTENT_EXTRACTION
	jsonRet[L"CONTENT"] = new JSONValue(m_wszContent.c_str());
#endif

	// Create a value
	JSONValue *value = new JSONValue(jsonRet);

	return value;
}

VOID CDEFile::SetFileName(TCHAR* wszName)
{
	_tcscpy(m_wszFileName, wszName);
}

VOID CDEFile::SetFileType(TCHAR* wszType)
{
	_tcscpy(m_wszFileType, wszType);
}

TCHAR* CDEFile::GetFileType()
{
	return m_wszFileType;
}
