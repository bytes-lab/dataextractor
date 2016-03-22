#include "StdAfx.h"

extern "C" {
#include "rtf/rtftype.h"
#include "rtf/rtfdecl.h"
}

#include "Util.h"
#include "RtfFile.h"

#ifdef SUPPORT_RTF

CRtfFile::CRtfFile(void)
{
	ZeroMemory(m_wszDateEdited, MAX_META_LEN * sizeof(TCHAR));
}

CRtfFile::~CRtfFile(void)
{
}

// RtfParseContent
INT RtfParseContent(FILE *fp, wstring *wsOutContent)
{
    INT ch;
    INT ec;
    INT cNibble = 2;
    INT b = 0;
    while ((ch = getc(fp)) != EOF)
    {
        if (cGroup < 0)
            return ecStackUnderflow;
        if (ris == risBin)                      // if we’re parsing binary data, handle it directly
        {
			if (ecCheckValidChar(ch) == ecOK)
				*wsOutContent += ch;
//            if ((ec = ecParseChar(ch)) != ecOK)
//                return ec;
        }
        else
        {
            switch (ch)
            {
            case '{':
                if ((ec = ecPushRtfState()) != ecOK)
                    return ec;
                break;
            case '}':
                if ((ec = ecPopRtfState()) != ecOK)
                    return ec;
                break;
            case '\\':
                if ((ec = ecParseRtfKeyword(fp)) != ecOK)
                    return ec;
                break;
            case 0x0d:
            case 0x0a:          // cr and lf are noise characters...
                break;
            default:
                if (ris == risNorm)
                {
					if (ecCheckValidChar(ch) == ecOK)
						*wsOutContent += ch;
//                    if ((ec = ecParseChar(ch)) != ecOK)
//                        return ec;
                }
                else
                {               // parsing hex data
                    if (ris != risHex)
                        return ecAssertion;
                    b = b << 4;
                    if (isdigit(ch))
                        b += (CHAR) ch - '0';
                    else
                    {
                        if (islower(ch))
                        {
                            if (ch < 'a' || ch > 'f')
                                return ecInvalidHex;
                            b += (CHAR) ch - 'a' + 10;
                        }
                        else
                        {
                            if (ch < 'A' || ch > 'F')
                                return ecInvalidHex;
                            b += (CHAR) ch - 'A' + 10;
                        }
                    }
                    cNibble--;
                    if (!cNibble)
                    {
                        cNibble = 2;
                        b = 0;
                        ris = risNorm;
                    }
                }                   // end else (ris != risNorm)
                break;
            }       // switch
        }           // else (ris != risBin)
    }               // while
    if (cGroup < 0)
        return ecStackUnderflow;
    if (cGroup > 0)
        return ecUnmatchedBrace;
    return ecOK;
}

BOOL CRtfFile::Load(TCHAR* wszFilePath)
{
	/*******************Read modified date***********************/
	SYSTEMTIME stEdited;
	FILETIME ftEdited = file::GetModificationTime( wszFilePath );

	::FileTimeToSystemTime(&ftEdited, &stEdited);
	_stprintf(m_wszDateEdited, L"%4d-%02d-%02d %02d:%02d:%02d", stEdited.wYear, stEdited.wMonth, stEdited.wDay, stEdited.wHour, stEdited.wMinute, stEdited.wSecond);

#ifdef CONTENT_EXTRACTION

	/********************Read content text***********************/
	FILE *fpDoc;
    int ec;

    fpDoc = _wfopen(wszFilePath, L"rb");
    if (!fpDoc)
    {
		_tprintf( L"A file is locked by another program: [%s]\n", wszFilePath);
        return 1;
    }
    if ((ec = RtfParseContent(fpDoc, &m_wszContent)) != ecOK)
		_tprintf( L"A file is invalid or damaged: [%s]\n", wszFilePath);

	fclose(fpDoc);
#endif

	return TRUE;
}

JSONValue* CRtfFile::Serialize(JSONObject *jsonNone)
{
	JSONObject *jsonMeta = new JSONObject;

	(*jsonMeta)[L"DATE EDITED"] = new JSONValue(m_wszDateEdited);

	return CDEFile::Serialize(jsonMeta);
}
#endif