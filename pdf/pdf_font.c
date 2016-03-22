#ifdef SUPPORT_PDF

#include "fitz.h"
#include "mupdf.h"

static char *base_font_names[14][7] =
{
	{ "Courier", "CourierNew", "CourierNewPSMT", NULL },
	{ "Courier-Bold", "CourierNew,Bold", "Courier,Bold",
		"CourierNewPS-BoldMT", "CourierNew-Bold", NULL },
	{ "Courier-Oblique", "CourierNew,Italic", "Courier,Italic",
		"CourierNewPS-ItalicMT", "CourierNew-Italic", NULL },
	{ "Courier-BoldOblique", "CourierNew,BoldItalic", "Courier,BoldItalic",
		"CourierNewPS-BoldItalicMT", "CourierNew-BoldItalic", NULL },
	{ "Helvetica", "ArialMT", "Arial", NULL },
	{ "Helvetica-Bold", "Arial-BoldMT", "Arial,Bold", "Arial-Bold",
		"Helvetica,Bold", NULL },
	{ "Helvetica-Oblique", "Arial-ItalicMT", "Arial,Italic", "Arial-Italic",
		"Helvetica,Italic", "Helvetica-Italic", NULL },
	{ "Helvetica-BoldOblique", "Arial-BoldItalicMT",
		"Arial,BoldItalic", "Arial-BoldItalic",
		"Helvetica,BoldItalic", "Helvetica-BoldItalic", NULL },
	{ "Times-Roman", "TimesNewRomanPSMT", "TimesNewRoman",
		"TimesNewRomanPS", NULL },
	{ "Times-Bold", "TimesNewRomanPS-BoldMT", "TimesNewRoman,Bold",
		"TimesNewRomanPS-Bold", "TimesNewRoman-Bold", NULL },
	{ "Times-Italic", "TimesNewRomanPS-ItalicMT", "TimesNewRoman,Italic",
		"TimesNewRomanPS-Italic", "TimesNewRoman-Italic", NULL },
	{ "Times-BoldItalic", "TimesNewRomanPS-BoldItalicMT",
		"TimesNewRoman,BoldItalic", "TimesNewRomanPS-BoldItalic",
		"TimesNewRoman-BoldItalic", NULL },
	{ "Symbol", NULL },
	{ "ZapfDingbats", NULL }
};

static int is_dynalab(char *name)
{
	if (strstr(name, "HuaTian"))
		return 1;
	if (strstr(name, "MingLi"))
		return 1;
	if ((strstr(name, "DF") == name) || strstr(name, "+DF"))
		return 1;
	if ((strstr(name, "DLC") == name) || strstr(name, "+DLC"))
		return 1;
	return 0;
}

static int strcmp_ignore_space(char *a, char *b)
{
	while (1)
	{
		while (*a == ' ')
			a++;
		while (*b == ' ')
			b++;
		if (*a != *b)
			return 1;
		if (*a == 0)
			return *a != *b;
		if (*b == 0)
			return *a != *b;
		a++;
		b++;
	}
}

static char *clean_font_name(char *fontname)
{
	int i, k;
	for (i = 0; i < 14; i++)
		for (k = 0; base_font_names[i][k]; k++)
			if (!strcmp_ignore_space(base_font_names[i][k], fontname))
				return base_font_names[i][0];
	return fontname;
}

/*
 * FreeType and Rendering glue
 */

enum { UNKNOWN, TYPE1, TRUETYPE };

static int ft_cid_to_gid(pdf_font_desc *fontdesc, int cid)
{
	if (fontdesc->to_ttf_cmap)
	{
		cid = pdf_lookup_cmap(fontdesc->to_ttf_cmap, cid);
		return cid;
	}

	if (fontdesc->cid_to_gid)
		return fontdesc->cid_to_gid[cid];

	return cid;
}

int
pdf_font_cid_to_gid(pdf_font_desc *fontdesc, int cid)
{
//	if (fontdesc->font->ft_face)
		return ft_cid_to_gid(fontdesc, cid);
	return cid;
}

static int lookup_mre_code(char *name)
{
	int i;
	for (i = 0; i < 256; i++)
		if (pdf_mac_roman[i] && !strcmp(name, pdf_mac_roman[i]))
			return i;
	return -1;
}

/*
 * Create and destroy
 */

pdf_font_desc *
pdf_keep_font(pdf_font_desc *fontdesc)
{
	fontdesc->refs ++;
	return fontdesc;
}

void
pdf_drop_font(pdf_font_desc *fontdesc)
{
	if (fontdesc && --fontdesc->refs == 0)
	{
		/* SumatraPDF: free vertical glyph substitution data (before font!) */
//		pdf_ft_free_vsubst(fontdesc);
		if (fontdesc->encoding)
			pdf_drop_cmap(fontdesc->encoding);
		if (fontdesc->to_ttf_cmap)
			pdf_drop_cmap(fontdesc->to_ttf_cmap);
		if (fontdesc->to_unicode)
			pdf_drop_cmap(fontdesc->to_unicode);
		fz_free(fontdesc->cid_to_gid);
		fz_free(fontdesc->cid_to_ucs);
		fz_free(fontdesc->hmtx);
		fz_free(fontdesc->vmtx);
		fz_free(fontdesc);
	}
}

pdf_font_desc *
pdf_new_font_desc(void)
{
	pdf_font_desc *fontdesc;

	fontdesc = fz_malloc(sizeof(pdf_font_desc));
	fontdesc->refs = 1;

	fontdesc->encoding = NULL;
	fontdesc->to_ttf_cmap = NULL;
	fontdesc->cid_to_gid_len = 0;
	fontdesc->cid_to_gid = NULL;

	fontdesc->to_unicode = NULL;
	fontdesc->cid_to_ucs_len = 0;
	fontdesc->cid_to_ucs = NULL;

	fontdesc->wmode = 0;

	fontdesc->hmtx_cap = 0;
	fontdesc->vmtx_cap = 0;
	fontdesc->hmtx_len = 0;
	fontdesc->vmtx_len = 0;
	fontdesc->hmtx = NULL;
	fontdesc->vmtx = NULL;

	fontdesc->dhmtx.lo = 0x0000;
	fontdesc->dhmtx.hi = 0xFFFF;
	fontdesc->dhmtx.w = 1000;

	fontdesc->dvmtx.lo = 0x0000;
	fontdesc->dvmtx.hi = 0xFFFF;
	fontdesc->dvmtx.x = 0;
	fontdesc->dvmtx.y = 880;
	fontdesc->dvmtx.w = -1000;

	fontdesc->is_embedded = 0;

	/* SumatraPDF: vertical glyph substitution */
	fontdesc->_vsubst = NULL;

	return fontdesc;
}

/*
 * Simple fonts (Type1 and TrueType)
 */

static fz_error
pdf_load_simple_font(pdf_font_desc **fontdescp, pdf_xref *xref, fz_obj *dict)
{
	fz_error error;
	fz_obj *encoding;
	unsigned short *etable = NULL;
	pdf_font_desc *fontdesc;

	char *basefont;
	char *fontname;
	char *estrings[256];
	int i, k, n;

	basefont = fz_to_name(fz_dict_gets(dict, "BaseFont"));
	fontname = clean_font_name(basefont);

	/* Load font file */

	fontdesc = pdf_new_font_desc();

	/* Encoding */

	encoding = fz_dict_gets(dict, "Encoding");
	if (encoding)
	{
		if (fz_is_name(encoding))
			pdf_load_encoding(estrings, fz_to_name(encoding));

		if (fz_is_dict(encoding))
		{
			fz_obj *base, *diff, *item;

			base = fz_dict_gets(encoding, "BaseEncoding");
			if (fz_is_name(base))
				pdf_load_encoding(estrings, fz_to_name(base));
			else if (!fontdesc->is_embedded)
				pdf_load_encoding(estrings, "StandardEncoding");

			diff = fz_dict_gets(encoding, "Differences");
			if (fz_is_array(diff))
			{
				n = fz_array_len(diff);
				k = 0;
				for (i = 0; i < n; i++)
				{
					item = fz_array_get(diff, i);
					if (fz_is_int(item))
						k = fz_to_int(item);
					if (fz_is_name(item))
						estrings[k++] = fz_to_name(item);
					if (k < 0) k = 0;
					if (k > 255) k = 255;
				}
			}
		}
	}

	fontdesc->encoding = pdf_new_identity_cmap(0, 1);
	fontdesc->cid_to_gid_len = 256;
	fontdesc->cid_to_gid = etable;

	error = pdf_load_to_unicode(fontdesc, xref, estrings, NULL, fz_dict_gets(dict, "ToUnicode"));
	if (error) {
		fz_catch(error, "cannot load to_unicode");
		goto cleanup;
	}

	*fontdescp = fontdesc;
	return fz_okay;

cleanup:
	if (etable != fontdesc->cid_to_gid)
		fz_free(etable);
	pdf_drop_font(fontdesc);
	return fz_rethrow(error, "cannot load simple font (%d %d R)", fz_to_num(dict), fz_to_gen(dict));
}

/*
 * CID Fonts
 */

static fz_error
load_cid_font(pdf_font_desc **fontdescp, pdf_xref *xref, fz_obj *dict, fz_obj *encoding, fz_obj *to_unicode)
{
	fz_error error;
	pdf_font_desc *fontdesc;

	fontdesc = pdf_new_font_desc();

	/* Encoding */
	
	error = fz_okay;
	if (fz_is_name(encoding))
	{
		if (!strcmp(fz_to_name(encoding), "Identity-H"))
			fontdesc->encoding = pdf_new_identity_cmap(0, 2);
		else if (!strcmp(fz_to_name(encoding), "Identity-V"))
			fontdesc->encoding = pdf_new_identity_cmap(1, 2);
		else
			error = pdf_load_system_cmap(&fontdesc->encoding, fz_to_name(encoding));
	}
	else if (fz_is_indirect(encoding))
	{
		error = pdf_load_embedded_cmap(&fontdesc->encoding, xref, encoding);
	}
	else
	{
		error = fz_throw("syntaxerror: font missing encoding");
	}
	if (error)
		goto cleanup;

	pdf_set_font_wmode(fontdesc, pdf_get_wmode(fontdesc->encoding));
	
	if (fz_is_name(encoding)) {
		error = pdf_load_to_unicode(fontdesc, xref, NULL, NULL, to_unicode);
		if (error)
			fz_catch(error, "cannot load to_unicode");
	}
	*fontdescp = fontdesc;
	return fz_okay;

cleanup:
	pdf_drop_font(fontdesc);
	return fz_rethrow(error, "cannot load cid font (%d %d R)", fz_to_num(dict), fz_to_gen(dict));
}

static fz_error
pdf_load_type0_font(pdf_font_desc **fontdescp, pdf_xref *xref, fz_obj *dict)
{
	fz_error error;
	fz_obj *dfonts;
	fz_obj *dfont;
	fz_obj *subtype;
	fz_obj *encoding;
	fz_obj *to_unicode;

	dfonts = fz_dict_gets(dict, "DescendantFonts");
//	if (!dfonts)
//		return fz_throw("cid font is missing descendant fonts");

	dfont = fz_array_get(dfonts, 0);
	
	subtype = fz_dict_gets(dfont, "Subtype");
	encoding = fz_dict_gets(dict, "Encoding");
	to_unicode = fz_dict_gets(dict, "ToUnicode");

//	if (fz_is_name(subtype) && !strcmp(fz_to_name(subtype), "CIDFontType0"))
	error = load_cid_font(fontdescp, xref, NULL, encoding, to_unicode);
//	else if (fz_is_name(subtype) && !strcmp(fz_to_name(subtype), "CIDFontType2"))
//		error = load_cid_font(fontdescp, xref, dfont, encoding, to_unicode);
//	else
//		error = fz_throw("syntaxerror: unknown cid font type");
//	if (error)
//		return fz_rethrow(error, "cannot load descendant font (%d %d R)", fz_to_num(dfont), fz_to_gen(dfont));

	return fz_okay;
}

fz_error
pdf_load_font(pdf_font_desc **fontdescp, pdf_xref *xref, fz_obj *rdb, fz_obj *dict)
{
	fz_error error;
	char *subtype;
	fz_obj *dfonts;
	fz_obj *charprocs;

	if ((*fontdescp = pdf_find_item(xref->store, pdf_drop_font, dict)))
	{
		pdf_keep_font(*fontdescp);
		return fz_okay;
	}

	subtype = fz_to_name(fz_dict_gets(dict, "Subtype"));
	dfonts = fz_dict_gets(dict, "DescendantFonts");
	charprocs = fz_dict_gets(dict, "CharProcs");

	if (subtype && !strcmp(subtype, "Type0"))
		error = pdf_load_type0_font(fontdescp, xref, dict);
	else if (subtype && !strcmp(subtype, "Type1"))
		error = pdf_load_simple_font(fontdescp, xref, dict);
	else if (subtype && !strcmp(subtype, "MMType1"))
		error = pdf_load_simple_font(fontdescp, xref, dict);
	else if (subtype && !strcmp(subtype, "TrueType"))
		error = pdf_load_simple_font(fontdescp, xref, dict);
	else if (dfonts)
	{
		fz_warn("unknown font format, guessing type0.");
		error = pdf_load_type0_font(fontdescp, xref, dict);
	}
	else
	{
		fz_warn("unknown font format, guessing type1 or truetype.");
		error = pdf_load_simple_font(fontdescp, xref, dict);
	}
	if (error)
		return fz_rethrow(error, "cannot load font (%d %d R)", fz_to_num(dict), fz_to_gen(dict));

	pdf_store_item(xref->store, pdf_keep_font, pdf_drop_font, dict, *fontdescp);

	return fz_okay;
}

void
pdf_debug_font(pdf_font_desc *fontdesc)
{
	int i;

	printf("fontdesc {\n");

	printf("\twmode %d\n", fontdesc->wmode);
	printf("\tDW %d\n", fontdesc->dhmtx.w);

	printf("\tW {\n");
	for (i = 0; i < fontdesc->hmtx_len; i++)
		printf("\t\t<%04x> <%04x> %d\n",
			fontdesc->hmtx[i].lo, fontdesc->hmtx[i].hi, fontdesc->hmtx[i].w);
	printf("\t}\n");

	if (fontdesc->wmode)
	{
		printf("\tDW2 [%d %d]\n", fontdesc->dvmtx.y, fontdesc->dvmtx.w);
		printf("\tW2 {\n");
		for (i = 0; i < fontdesc->vmtx_len; i++)
			printf("\t\t<%04x> <%04x> %d %d %d\n", fontdesc->vmtx[i].lo, fontdesc->vmtx[i].hi,
				fontdesc->vmtx[i].x, fontdesc->vmtx[i].y, fontdesc->vmtx[i].w);
		printf("\t}\n");
	}
}

#endif