#include "StdAfx.h"

#ifdef SUPPORT_PDF

#include "PdfFile.h"
#include "Util.h"

#include <exiv2.hpp>
#include "zlib/zlib.h"

CPdfFile::CPdfFile(void)
{
	ZeroMemory(m_wszAuthor, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszTitle, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszSubject, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszKeywords, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszCreated, MAX_META_LEN * sizeof(TCHAR));
	ZeroMemory(m_wszDateEdited, MAX_META_LEN * sizeof(TCHAR));

	m_wszXmp.clear();
	m_wszCommentAndReview.clear();
	m_wszAttachments.clear();		
}

CPdfFile::~CPdfFile(void)
{
}

VOID CPdfFile::GetProperty(CHAR *szName, TCHAR *wszResult) 
{
    if (!m_xrfDoc)
        return;

    if (!strcmp(szName, "PdfVersion")) {
        int major = m_xrfDoc->version / 10, minor = m_xrfDoc->version % 10;
        if (1 == major && 7 == minor && 5 == pdf_get_crypt_revision(m_xrfDoc)) {
            _stprintf(wszResult, L"%d.%d Adobe Extension Level %d", major, minor, 3);
			return;
		}

        _stprintf(wszResult, L"%d.%d", major, minor);
		return;
    }

    // _info is guaranteed not to be an indirect reference, so no need for _xrefAccess
    fz_obj *obj = fz_dict_gets(m_infDoc, szName);
    if (!obj)
        return;

	if(fz_is_string(obj)) {
		WCHAR *ucs2 = (WCHAR *)pdf_to_ucs2(obj);
		_tcscpy(wszResult, ucs2);
		fz_free(ucs2);
	}
}

pdf_link *pdf_new_link(fz_obj *dest, pdf_link_kind kind, fz_rect rect=fz_empty_rect)
{
    pdf_link *link = (pdf_link *)fz_malloc(sizeof(pdf_link));

    link->dest = dest;
    link->kind = kind;
    link->rect = rect;
    link->next = NULL;

    return link;
}

// note: make sure to only call with _xrefAccess
pdf_outline *pdf_loadattachments(pdf_xref *xref)
{
    fz_obj *dict = pdf_load_name_tree(xref, "EmbeddedFiles");
    if (!dict)
        return NULL;

    pdf_outline root = { 0 }, *node = &root;
    for (int i = 0; i < fz_dict_len(dict); i++) {
        node = node->next = (pdf_outline *)fz_malloc(sizeof(pdf_outline));
        ZeroMemory(node, sizeof(pdf_outline));

        fz_obj *name = fz_dict_get_key(dict, i);
        fz_obj *dest = fz_dict_get_val(dict, i);
        fz_obj *type = fz_dict_gets(dest, "Type");

        node->title = fz_strdup(fz_to_name(name));
        if (fz_is_name(type) && !_stricmp(fz_to_name(type), "Filespec"))
            node->link = pdf_new_link(fz_keep_obj(dest), PDF_LINK_LAUNCH);
    }
    fz_drop_obj(dict);

    return root.next;
}

fz_buffer *
pdf_load_xmp_data(pdf_xref *xref)
{
	fz_obj *root = fz_dict_gets(xref->trailer, "Root");
	fz_obj *meta = fz_dict_gets(root, "Metadata");

	fz_error error = fz_okay;
	fz_buffer *buf = fz_new_buffer(32 * 1024);
	error = pdf_load_stream(&buf, xref, fz_to_num(meta), fz_to_gen(meta));
	if (error)
	{
		fz_drop_buffer(buf);
		return NULL;
	}

	return buf;
}

WCHAR *fz_span_to_wchar(fz_text_span *text, TCHAR *lineSep)
{
    size_t lineSepLen = _tcslen(lineSep);
    size_t textLen = 0;
    for (fz_text_span *span = text; span; span = span->next)
        textLen += span->len + lineSepLen;

    WCHAR *content = (WCHAR *) calloc(textLen + 1, sizeof(WCHAR));
    if (!content)
        return NULL;

    WCHAR *dest = content;
    for (fz_text_span *span = text; span; span = span->next) {
        for (int i = 0; i < span->len; i++) {
            *dest = span->text[i].c;
            if (*dest < 32)
                *dest = '?';
            dest++;
        }
        if (!span->eol && span->next)
            continue;
#ifdef UNICODE
        lstrcpy(dest, lineSep);
        dest += lineSepLen;
#else
        dest += MultiByteToWideChar(CP_ACP, 0, lineSep, -1, dest, lineSepLen + 1);
#endif
    }

    return content;
}

BOOL CPdfFile::Load(TCHAR* wszFilePath)
{
	/*********************load pdf XRef and Info Dictionaries******************************/
    fz_stream *fsInput = NULL;

	//Get the file length:
	size_t fileSize = file::GetSize(wszFilePath);

	// load small files entirely into memory so that they can be
	// overwritten even by programs that don't open files with FILE_SHARE_READ
	if (fileSize < MAX_MEMORY_FILE_SIZE) {
		fz_buffer *data = fz_new_buffer((int)fileSize);
		if (data) {
			if (file::ReadAll(wszFilePath, (char *)data->data, (data->len = (int)fileSize)))
				fsInput = fz_open_buffer(data);
			fz_drop_buffer(data);
		}
	}

    if (!fsInput) {
		_tprintf( L"A file is locked by another program: [%s]\n", wszFilePath);
        return FALSE;
	}

    // don't pass in a password so that _xref isn't thrown away if it was wrong
    fz_error error = pdf_open_xref_with_stream(&m_xrfDoc, fsInput, NULL);
    fz_close(fsInput);
    if (error || !m_xrfDoc) {
		_tprintf( L"A file is invalid or damaged: [%s]\n", wszFilePath);
        return FALSE;
	}

    if (pdf_needs_password(m_xrfDoc)) {
		_tprintf(L"A pdf file is encrypted: %s\n", wszFilePath);
        return false;
    }

    m_infDoc = fz_dict_gets(m_xrfDoc->trailer, "Info");
    if (m_infDoc)
        m_infDoc = fz_copy_dict(pdf_resolve_indirect(m_infDoc));

	// Fetch Properties
	GetProperty("Title", m_wszTitle);
	GetProperty("Subject", m_wszSubject);
	GetProperty("Author", m_wszAuthor);
	GetProperty("Keywords", m_wszKeywords);
	GetProperty("CreationDate", m_wszCreated);
	GetProperty("ModDate", m_wszDateEdited);

	/*********************************load pdf Attachments**************************************/
    pdf_outline *atchsDoc = pdf_loadattachments(m_xrfDoc);
    for (; atchsDoc; atchsDoc = atchsDoc->next) {
		if(atchsDoc->title != NULL) {
			TCHAR wszAttachTitle[MAX_META_LEN] = L"";
			INT nLen = 0;
			nLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, atchsDoc->title, -1, NULL, NULL);
			MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, atchsDoc->title, strlen(atchsDoc->title)+1, wszAttachTitle, nLen);
			m_wszAttachments += wszAttachTitle;
			m_wszAttachments += L" ";
		}
    }

	/*********************************load pdf XMP meta data*************************************/
	fz_buffer *bufXmp = pdf_load_xmp_data(m_xrfDoc);
	if(bufXmp) {
		TCHAR wszXmp[65535] = L"";
		INT nLen = 0;
		nLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR) bufXmp->data, bufXmp->len, NULL, NULL);
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (LPCSTR) bufXmp->data, bufXmp->len, wszXmp, nLen);
		m_wszXmp += wszXmp;
		m_wszXmp += L" ";
		fz_drop_buffer(bufXmp);
	}

#ifdef CONTENT_EXTRACTION

	/*********************load pdf Comment&Review and Content text*******************************/
	fz_error erRet = pdf_load_page_tree(m_xrfDoc);
	if (erRet) {
		_tprintf( L"A file is invalid or damaged: [%s]\n", wszFilePath);
		return FALSE;
	}

    if (pdf_count_pages(m_xrfDoc) == 0) {
		_tprintf( L"A file is invalid or damaged: [%s]\n", wszFilePath);
		return FALSE;
	}

	for (int nPageIndex = 1; nPageIndex <= pdf_count_pages(m_xrfDoc); nPageIndex++) {

		pdf_page *pgTmp = NULL;
		WCHAR *wszContent = NULL;

		erRet = pdf_load_page(&pgTmp, m_xrfDoc, nPageIndex - 1);

		for (pdf_annot *annot = pgTmp->annots; annot; annot = annot->next) {
			WCHAR *ucs2 = (WCHAR *)pdf_to_ucs2(fz_dict_gets(annot->obj, "Contents"));
			m_wszCommentAndReview += ucs2;
			m_wszCommentAndReview += L" ";
			fz_free(ucs2);
		}

		if (!erRet && pgTmp) {

			fz_text_span *tsTmp = fz_new_text_span();
			erRet = pdf_run_page_with_usage(m_xrfDoc, pgTmp, fz_new_text_device(tsTmp), fz_identity, "Export");

			if (erRet) {
				fz_free_text_span(tsTmp);
				continue;
			}

			wszContent = fz_span_to_wchar(tsTmp, _T("\n"));

			// append content text
			m_wszContent += wszContent;

			free(wszContent);
			fz_free_text_span(tsTmp);
			pdf_free_page(pgTmp);

		} else {
			_tprintf( L"A file is invalid or damaged: [%s]\n", wszFilePath);
			break;
		}
	}
#endif

	return TRUE;
}

JSONValue* CPdfFile::Serialize(JSONObject *jsonNone)
{
	JSONObject *jsonMeta = new JSONObject;

	(*jsonMeta)[L"AUTHOR"] = new JSONValue(m_wszAuthor);
	(*jsonMeta)[L"TITLE"] = new JSONValue(m_wszTitle);
	(*jsonMeta)[L"SUBJECT"] = new JSONValue(m_wszSubject);
	(*jsonMeta)[L"KEYWORDS"] = new JSONValue(m_wszKeywords);
	(*jsonMeta)[L"CREATED"] = new JSONValue(m_wszCreated);
	(*jsonMeta)[L"DATE EDITED"] = new JSONValue(m_wszDateEdited);

	(*jsonMeta)[L"XMP"] = new JSONValue(m_wszXmp.c_str());
	(*jsonMeta)[L"REVIEW & COMMENTING"] = new JSONValue(m_wszCommentAndReview.c_str());
	(*jsonMeta)[L"ATTACHMENT & EMBEDDING"] = new JSONValue(m_wszAttachments.c_str());

	return CDEFile::Serialize(jsonMeta);
}

#endif