#include "StdAfx.h"

#include "OfficeFile.h"
#include "Json/JSON.h"

COfficeFile::COfficeFile(void)
{
	ZeroMemory(m_wszAuthor, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszTitle, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszSubject, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszKeywords, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszStatus, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszCompany, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszApplication, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszComments, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszTemplateUsed, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszRevisionNumber, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszTotalEditingTime, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszLastSaved, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszLastEditedBy, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszLastPrinted, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszRecentHyperlinksList, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszPageCount, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszWordCount, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszCharacterCount, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszTrackChanges, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszGUID, MAX_META_LEN * sizeof(TCHAR));
}

COfficeFile::~COfficeFile(void)
{
}

JSONValue* COfficeFile::Serialize(JSONObject* jsonCustMeta)
{
	JSONObject *jsonMeta;

	if(jsonCustMeta != NULL)
		jsonMeta = jsonCustMeta;
	else
		jsonMeta = new JSONObject;

	(*jsonMeta)[L"AUTHOR"] = new JSONValue(m_wszAuthor);
	(*jsonMeta)[L"TITLE"] = new JSONValue(m_wszTitle);
	(*jsonMeta)[L"SUBJECT"] = new JSONValue(m_wszSubject);
	(*jsonMeta)[L"KEYWORDS"] = new JSONValue(m_wszKeywords);
	(*jsonMeta)[L"STATUS"] = new JSONValue(m_wszStatus);
	(*jsonMeta)[L"COMPANY"] = new JSONValue(m_wszCompany);
	(*jsonMeta)[L"APPLICATION"] = new JSONValue(m_wszApplication);
	(*jsonMeta)[L"COMMENTS"] = new JSONValue(m_wszComments);
	(*jsonMeta)[L"TEMPLATE USED"] = new JSONValue(m_wszTemplateUsed);
	(*jsonMeta)[L"REVISION NUMBER"] = new JSONValue(m_wszRevisionNumber);
	(*jsonMeta)[L"TOTAL EDITING TIME"] = new JSONValue(m_wszTotalEditingTime);
	(*jsonMeta)[L"LAST SAVED"] = new JSONValue(m_wszLastSaved);
	(*jsonMeta)[L"LAST EDITED BY"] = new JSONValue(m_wszLastEditedBy);
	(*jsonMeta)[L"LAST PRINTED"] = new JSONValue(m_wszLastPrinted);
	(*jsonMeta)[L"RECENT HYPERLINKS LINKS"] = new JSONValue(m_wszRecentHyperlinksList);
	(*jsonMeta)[L"PAGE COUNT"] = new JSONValue(m_wszPageCount);
	(*jsonMeta)[L"WORD COUNT"] = new JSONValue(m_wszWordCount);
	(*jsonMeta)[L"CHARACTER COUNT"] = new JSONValue(m_wszCharacterCount);
	(*jsonMeta)[L"TRACK CHANGES"] = new JSONValue(m_wszTrackChanges);
	(*jsonMeta)[L"GUID"] = new JSONValue(m_wszGUID);

	return CDEFile::Serialize(jsonMeta);
}
