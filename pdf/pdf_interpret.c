#ifdef SUPPORT_PDF

#include "fitz.h"
#include "mupdf.h"

#define TILE

typedef struct pdf_material_s pdf_material;
typedef struct pdf_gstate_s pdf_gstate;
typedef struct pdf_csi_s pdf_csi;

enum
{
	PDF_FILL,
	PDF_STROKE,
};

enum
{
	PDF_MAT_NONE,
	PDF_MAT_COLOR,
	PDF_MAT_PATTERN,
	PDF_MAT_SHADE,
};

struct pdf_material_s
{
	int kind;
	fz_colorspace *colorspace;
	pdf_pattern *pattern;
	fz_shade *shade;
	float alpha;
	float v[32];
};

struct pdf_gstate_s
{
	fz_matrix ctm;
	int clip_depth;

	/* text state */
	float char_space;
	float word_space;
	float scale;
	float leading;
	pdf_font_desc *font;
	float size;
	int render;
	float rise;
};

struct pdf_csi_s
{
	fz_device *dev;
	pdf_xref *xref;

	/* usage mode for optional content groups */
	char *target; /* "View", "Print", "Export" */

	/* interpreter stack */
	fz_obj *obj;
	char name[256];
	unsigned char string[256];
	int string_len;
	float stack[32];
	int top;

	int xbalance;
	int in_text;
	int in_hidden_ocg; /* SumatraPDF: support inline OCGs */

	/* path object state */
	fz_path *path;
	/* cf. http://bugs.ghostscript.com/show_bug.cgi?id=692391 */
	int clip; /* 0: none, 1: winding, 2: even-odd */

	/* text object state */
	fz_text *text;
	fz_matrix tlm;
	fz_matrix tm;
	int text_mode;
	int accumulate;

	/* graphics state */
	fz_matrix top_ctm;
	/* SumatraPDF: make gstate growable for rendering tikz-qtree documents */
	pdf_gstate *gstate;
	int gcap;
	int gtop;
};

static void pdf_show_clip(pdf_csi *csi, int even_odd)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	gstate->clip_depth++;
	fz_clip_path(csi->dev, csi->path, NULL, even_odd, gstate->ctm);
}

/*
 * Assemble and emit text
 */

static void
pdf_flush_text(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_text *text;
	int dofill = 0;
	int dostroke = 0;
	int doclip = 0;
	int doinvisible = 0;
	fz_rect bbox;

	if (!csi->text)
		return;
	text = csi->text;
	csi->text = NULL;

	dofill = dostroke = doclip = doinvisible = 0;
	switch (csi->text_mode)
	{
	case 0: dofill = 1; break;
	case 1: dostroke = 1; break;
	case 2: dofill = dostroke = 1; break;
	case 3: doinvisible = 1; break;
	case 4: dofill = doclip = 1; break;
	case 5: dostroke = doclip = 1; break;
	case 6: dofill = dostroke = doclip = 1; break;
	case 7: doclip = 1; break;
	}

	/* SumatraPDF: support inline OCGs */
	if (csi->in_hidden_ocg > 0)
		dofill = dostroke = 0;

	bbox = fz_bound_text(text, gstate->ctm);

	if (doinvisible)
		fz_ignore_text(csi->dev, text, gstate->ctm);

	if (doclip)
	{
		if (csi->accumulate < 2)
			gstate->clip_depth++;
		fz_clip_text(csi->dev, text, gstate->ctm, csi->accumulate);
		csi->accumulate = 2;
	}

	if (dofill)
	{
		fz_fill_text(csi->dev, text, gstate->ctm,
			0, 0, 0);
	}

	fz_free_text(text);
}

static void
pdf_show_char(pdf_csi *csi, int cid)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;
	fz_matrix tsm, trm;
	float w0, w1, tx, ty;
	pdf_hmtx h;
	pdf_vmtx v;
	int gid;
	int ucsbuf[8];
	int ucslen;
	int i;

	tsm.a = gstate->size * gstate->scale;
	tsm.b = 0;
	tsm.c = 0;
	tsm.d = gstate->size;
	tsm.e = 0;
	tsm.f = gstate->rise;

	ucslen = 0;
	if (fontdesc->to_unicode)
		ucslen = pdf_lookup_cmap_full(fontdesc->to_unicode, cid, ucsbuf);
	if (ucslen == 0 && cid < fontdesc->cid_to_ucs_len)
	{
		ucsbuf[0] = fontdesc->cid_to_ucs[cid];
		ucslen = 1;
	}
	if (ucslen == 0 || (ucslen == 1 && ucsbuf[0] == 0))
	{
		ucsbuf[0] = '?';
		ucslen = 1;
	}

	gid = cid;//pdf_font_cid_to_gid(fontdesc, cid);todo

	/* cf. http://code.google.com/p/sumatrapdf/issues/detail?id=1149 */
//	if (fontdesc->wmode == 1)
//		gid = pdf_ft_get_vgid(fontdesc, gid);

	if (fontdesc->wmode == 1)
	{
		v = pdf_get_vmtx(fontdesc, cid);
		tsm.e -= v.x * gstate->size * 0.001f;
		tsm.f -= v.y * gstate->size * 0.001f;
	}

	trm = fz_concat(tsm, csi->tm);

	/* flush buffered text if face or matrix or rendermode has changed */
	if (!csi->text ||
		fontdesc->wmode != csi->text->wmode ||
		fabsf(trm.a - csi->text->trm.a) > FLT_EPSILON ||
		fabsf(trm.b - csi->text->trm.b) > FLT_EPSILON ||
		fabsf(trm.c - csi->text->trm.c) > FLT_EPSILON ||
		fabsf(trm.d - csi->text->trm.d) > FLT_EPSILON ||
		gstate->render != csi->text_mode)
	{
		pdf_flush_text(csi);

		csi->text = fz_new_text(NULL, trm, fontdesc->wmode);
		csi->text->trm.e = 0;
		csi->text->trm.f = 0;
		csi->text_mode = gstate->render;
	}

	/* add glyph to textobject */
	fz_add_text(csi->text, gid, ucsbuf[0], trm.e, trm.f);

	/* add filler glyphs for one-to-many unicode mapping */
	for (i = 1; i < ucslen; i++)
		fz_add_text(csi->text, -1, ucsbuf[i], trm.e, trm.f);

	if (fontdesc->wmode == 0)
	{
		h = pdf_get_hmtx(fontdesc, cid);
		w0 = h.w * 0.001f;
		tx = (w0 * gstate->size + gstate->char_space) * gstate->scale;
		csi->tm = fz_concat(fz_translate(tx, 0), csi->tm);
	}

	if (fontdesc->wmode == 1)
	{
		w1 = v.w * 0.001f;
		ty = w1 * gstate->size + gstate->char_space;
		csi->tm = fz_concat(fz_translate(0, ty), csi->tm);
	}
}

static void
pdf_show_space(pdf_csi *csi, float tadj)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;

	if (!fontdesc)
	{
		fz_warn("cannot draw text since font and size not set");
		return;
	}

	if (fontdesc->wmode == 0)
		csi->tm = fz_concat(fz_translate(tadj * gstate->scale, 0), csi->tm);
	else
		csi->tm = fz_concat(fz_translate(0, tadj), csi->tm);
}

static void
pdf_show_string(pdf_csi *csi, unsigned char *buf, int len)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_font_desc *fontdesc = gstate->font;
	unsigned char *end = buf + len;
	int cpt, cid;

	if (!fontdesc)
	{
		fz_warn("cannot draw text since font and size not set");
		return;
	}

	while (buf < end)
	{
		buf = pdf_decode_cmap(fontdesc->encoding, buf, &cpt);
		cid = pdf_lookup_cmap(fontdesc->encoding, cpt);
		if (cid >= 0)
			pdf_show_char(csi, cid);
		else
			fz_warn("cannot encode character with code point %#x", cpt);
		if (cpt == 32)
			pdf_show_space(csi, gstate->word_space);
	}
}

static void
pdf_show_text(pdf_csi *csi, fz_obj *text)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	int i;

	if (fz_is_array(text))
	{
		for (i = 0; i < fz_array_len(text); i++)
		{
			fz_obj *item = fz_array_get(text, i);
			if (fz_is_string(item))
				pdf_show_string(csi, (unsigned char *)fz_to_str_buf(item), fz_to_str_len(item));
			else
				pdf_show_space(csi, - fz_to_real(item) * gstate->size * 0.001f);
		}
	}
	else if (fz_is_string(text))
	{
		pdf_show_string(csi, (unsigned char *)fz_to_str_buf(text), fz_to_str_len(text));
	}
}

/*
 * Interpreter and graphics state stack.
 */

static void
pdf_init_gstate(pdf_gstate *gs, fz_matrix ctm)
{
	gs->ctm = ctm;
	gs->clip_depth = 0;

	gs->char_space = 0;
	gs->word_space = 0;
	gs->scale = 1;
	gs->leading = 0;
	gs->font = NULL;
	gs->size = -1;
	gs->render = 0;
	gs->rise = 0;
}

static pdf_csi *
pdf_new_csi(pdf_xref *xref, fz_device *dev, fz_matrix ctm, char *target)
{
	pdf_csi *csi;

	csi = fz_malloc(sizeof(pdf_csi));
	csi->xref = xref;
	csi->dev = dev;
	csi->target = target;

	csi->top = 0;
	csi->obj = NULL;
	csi->name[0] = 0;
	csi->string_len = 0;
	memset(csi->stack, 0, sizeof csi->stack);

	csi->xbalance = 0;
	csi->in_text = 0;
	csi->in_hidden_ocg = 0; /* SumatraPDF: support inline OCGs */

	csi->path = fz_new_path();
	csi->clip = 0; /* cf. http://bugs.ghostscript.com/show_bug.cgi?id=692391 */

	csi->text = NULL;
	csi->tlm = fz_identity;
	csi->tm = fz_identity;
	csi->text_mode = 0;
	csi->accumulate = 1;

	/* SumatraPDF: make gstate growable for rendering tikz-qtree documents */
	csi->gcap = 32;
	csi->gstate = fz_calloc(csi->gcap, sizeof(pdf_gstate));

	csi->top_ctm = ctm;
	pdf_init_gstate(&csi->gstate[0], ctm);
	csi->gtop = 0;

	return csi;
}

static void
pdf_clear_stack(pdf_csi *csi)
{
	int i;

	if (csi->obj)
		fz_drop_obj(csi->obj);
	csi->obj = NULL;

	csi->name[0] = 0;
	csi->string_len = 0;
	for (i = 0; i < csi->top; i++)
		csi->stack[i] = 0;

	csi->top = 0;
}

static void
pdf_gsave(pdf_csi *csi)
{
	pdf_gstate *gs = csi->gstate + csi->gtop;

	/* SumatraPDF: make gstate growable for rendering tikz-qtree documents */
	if (csi->gtop == csi->gcap - 1)
	{
		fz_warn("gstate overflow in content stream");
		csi->gcap *= 2;
		csi->gstate = fz_realloc(csi->gstate, csi->gcap, sizeof(pdf_gstate));
		gs = csi->gstate + csi->gtop;
	}

	memcpy(&csi->gstate[csi->gtop + 1], &csi->gstate[csi->gtop], sizeof(pdf_gstate));

	csi->gtop ++;

	if (gs->font)
		pdf_keep_font(gs->font);
}

static void
pdf_grestore(pdf_csi *csi)
{
	pdf_gstate *gs = csi->gstate + csi->gtop;
	int clip_depth = gs->clip_depth;

	if (csi->gtop == 0)
	{
		fz_warn("gstate underflow in content stream");
		return;
	}

	if (gs->font)
		pdf_drop_font(gs->font);

	csi->gtop --;

	gs = csi->gstate + csi->gtop;
	while (clip_depth > gs->clip_depth)
	{
		fz_pop_clip(csi->dev);
		clip_depth--;
	}
}

static void
pdf_free_csi(pdf_csi *csi)
{
	while (csi->gtop)
		pdf_grestore(csi);

	if (csi->gstate[0].font)
		pdf_drop_font(csi->gstate[0].font);

	while (csi->gstate[0].clip_depth--)
		fz_pop_clip(csi->dev);

	if (csi->path) fz_free_path(csi->path);
	if (csi->text) fz_free_text(csi->text);

	pdf_clear_stack(csi);

	/* SumatraPDF: make gstate growable for rendering tikz-qtree documents */
	fz_free(csi->gstate);

	fz_free(csi);
}

static fz_error
pdf_run_extgstate(pdf_csi *csi, fz_obj *rdb, fz_obj *extgstate)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	int i;

	pdf_flush_text(csi);

	for (i = 0; i < fz_dict_len(extgstate); i++)
	{
		fz_obj *key = fz_dict_get_key(extgstate, i);
		fz_obj *val = fz_dict_get_val(extgstate, i);
		char *s = fz_to_name(key);

		if (!strcmp(s, "Font"))
		{
			if (fz_is_array(val) && fz_array_len(val) == 2)
			{
				fz_error error;
				fz_obj *font = fz_array_get(val, 0);

				if (gstate->font)
				{
					pdf_drop_font(gstate->font);
					gstate->font = NULL;
				}

				error = pdf_load_font(&gstate->font, csi->xref, rdb, font);
				if (error)
					return fz_rethrow(error, "cannot load font (%d %d R)", fz_to_num(font), fz_to_gen(font));
				if (!gstate->font)
					return fz_throw("cannot find font in store");
				gstate->size = fz_to_real(fz_array_get(val, 1));
			}
			else
				return fz_throw("malformed /Font dictionary");
		}

		else if (!strcmp(s, "TR"))
		{
			if (!fz_is_name(val) || strcmp(fz_to_name(val), "Identity"))
				fz_warn("ignoring transfer function");
		}
	}

	return fz_okay;
}

static void pdf_run_BMC(pdf_csi *csi)
{
	/* SumatraPDF: support inline OCGs */
	if (csi->in_hidden_ocg > 0)
		csi->in_hidden_ocg++;
}

static void pdf_run_BT(pdf_csi *csi)
{
	csi->in_text = 1;
	csi->tm = fz_identity;
	csi->tlm = fz_identity;
}

static void pdf_run_BX(pdf_csi *csi)
{
	csi->xbalance ++;
}

static void pdf_run_DP(pdf_csi *csi)
{
}

static void pdf_run_ET(pdf_csi *csi)
{
	pdf_flush_text(csi);
	csi->accumulate = 1;
	csi->in_text = 0;
}

static void pdf_run_EX(pdf_csi *csi)
{
	csi->xbalance --;
}

static void pdf_run_J(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
}

static void pdf_run_M(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
}

static void pdf_run_MP(pdf_csi *csi)
{
}

static void pdf_run_Q(pdf_csi *csi)
{
	pdf_grestore(csi);
}

static void pdf_run_Tc(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->char_space = csi->stack[0];
}

static void pdf_run_Tw(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->word_space = csi->stack[0];
}

static void pdf_run_Tz(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	float a = csi->stack[0] / 100;
	pdf_flush_text(csi);
	gstate->scale = a;
}

static void pdf_run_TL(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->leading = csi->stack[0];
}

static fz_error pdf_run_Tf(pdf_csi *csi, fz_obj *rdb)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_error error;
	fz_obj *dict;
	fz_obj *obj;

	gstate->size = csi->stack[0];
	if (gstate->font)
		pdf_drop_font(gstate->font);
	gstate->font = NULL;

	dict = fz_dict_gets(rdb, "Font");
	if (!dict)
		return fz_throw("cannot find Font dictionary");

	obj = fz_dict_gets(dict, csi->name);
	if (!obj)
		return fz_throw("cannot find font resource: '%s'", csi->name);

	error = pdf_load_font(&gstate->font, csi->xref, rdb, obj);
	if (error)
		return fz_rethrow(error, "cannot load font (%d 0 R)", fz_to_num(obj));

	return fz_okay;
}

static void pdf_run_Tr(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->render = csi->stack[0];
}

static void pdf_run_Ts(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	gstate->rise = csi->stack[0];
}

static void pdf_run_Td(pdf_csi *csi)
{
	fz_matrix m = fz_translate(csi->stack[0], csi->stack[1]);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;
}

static void pdf_run_TD(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_matrix m;

	gstate->leading = -csi->stack[1];
	m = fz_translate(csi->stack[0], csi->stack[1]);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;
}

static void pdf_run_Tm(pdf_csi *csi)
{
	csi->tm.a = csi->stack[0];
	csi->tm.b = csi->stack[1];
	csi->tm.c = csi->stack[2];
	csi->tm.d = csi->stack[3];
	csi->tm.e = csi->stack[4];
	csi->tm.f = csi->stack[5];
	csi->tlm = csi->tm;
}

static void pdf_run_Tstar(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_matrix m = fz_translate(0, -gstate->leading);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;
}

static void pdf_run_Tj(pdf_csi *csi)
{
	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

static void pdf_run_TJ(pdf_csi *csi)
{
	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

static void pdf_run_W(pdf_csi *csi)
{
	csi->clip = 1; /* cf. http://bugs.ghostscript.com/show_bug.cgi?id=692391 */
}

static void pdf_run_Wstar(pdf_csi *csi)
{
	csi->clip = 2; /* cf. http://bugs.ghostscript.com/show_bug.cgi?id=692391 */
}

static void pdf_run_c(pdf_csi *csi)
{
	float a, b, c, d, e, f;
	a = csi->stack[0];
	b = csi->stack[1];
	c = csi->stack[2];
	d = csi->stack[3];
	e = csi->stack[4];
	f = csi->stack[5];
	fz_curveto(csi->path, a, b, c, d, e, f);
}

static void pdf_run_cm(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_matrix m;

	m.a = csi->stack[0];
	m.b = csi->stack[1];
	m.c = csi->stack[2];
	m.d = csi->stack[3];
	m.e = csi->stack[4];
	m.f = csi->stack[5];

	gstate->ctm = fz_concat(m, gstate->ctm);
}

static void pdf_run_d0(pdf_csi *csi)
{
	csi->dev->flags |= FZ_CHARPROC_COLOR;
}

static void pdf_run_d1(pdf_csi *csi)
{
	csi->dev->flags |= FZ_CHARPROC_MASK;
}

static fz_error pdf_run_gs(pdf_csi *csi, fz_obj *rdb)
{
	fz_error error;
	fz_obj *dict;
	fz_obj *obj;

	dict = fz_dict_gets(rdb, "ExtGState");
	if (!dict)
		return fz_throw("cannot find ExtGState dictionary");

	obj = fz_dict_gets(dict, csi->name);
	if (!obj)
		return fz_throw("cannot find extgstate resource '%s'", csi->name);

	error = pdf_run_extgstate(csi, rdb, obj);
	if (error)
		return fz_rethrow(error, "cannot set ExtGState (%d 0 R)", fz_to_num(obj));
	return fz_okay;
}

static void pdf_run_h(pdf_csi *csi)
{
	fz_closepath(csi->path);
}

static void pdf_run_i(pdf_csi *csi)
{
}

static void pdf_run_j(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
}

static void pdf_run_l(pdf_csi *csi)
{
	float a, b;
	a = csi->stack[0];
	b = csi->stack[1];
	fz_lineto(csi->path, a, b);
}

static void pdf_run_m(pdf_csi *csi)
{
	float a, b;
	a = csi->stack[0];
	b = csi->stack[1];
	fz_moveto(csi->path, a, b);
}

static void pdf_run_q(pdf_csi *csi)
{
	pdf_gsave(csi);
}

static void pdf_run_re(pdf_csi *csi)
{
	float x, y, w, h;

	x = csi->stack[0];
	y = csi->stack[1];
	w = csi->stack[2];
	h = csi->stack[3];

	fz_moveto(csi->path, x, y);
	fz_lineto(csi->path, x + w, y);
	fz_lineto(csi->path, x + w, y + h);
	fz_lineto(csi->path, x, y + h);
	fz_closepath(csi->path);
}

static void pdf_run_ri(pdf_csi *csi)
{
}

static void pdf_run_v(pdf_csi *csi)
{
	float a, b, c, d;
	a = csi->stack[0];
	b = csi->stack[1];
	c = csi->stack[2];
	d = csi->stack[3];
	fz_curvetov(csi->path, a, b, c, d);
}

static void pdf_run_w(pdf_csi *csi)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_flush_text(csi); /* linewidth affects stroked text rendering mode */
}

static void pdf_run_y(pdf_csi *csi)
{
	float a, b, c, d;
	a = csi->stack[0];
	b = csi->stack[1];
	c = csi->stack[2];
	d = csi->stack[3];
	fz_curvetoy(csi->path, a, b, c, d);
}

static void pdf_run_squote(pdf_csi *csi)
{
	fz_matrix m;
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	m = fz_translate(0, -gstate->leading);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;

	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

static void pdf_run_dquote(pdf_csi *csi)
{
	fz_matrix m;
	pdf_gstate *gstate = csi->gstate + csi->gtop;

	gstate->word_space = csi->stack[0];
	gstate->char_space = csi->stack[1];

	m = fz_translate(0, -gstate->leading);
	csi->tlm = fz_concat(m, csi->tlm);
	csi->tm = csi->tlm;

	if (csi->string_len)
		pdf_show_string(csi, csi->string, csi->string_len);
	else
		pdf_show_text(csi, csi->obj);
}

#define A(a) (a)
#define B(a,b) (a | b << 8)
#define C(a,b,c) (a | b << 8 | c << 16)

static fz_error
pdf_run_keyword(pdf_csi *csi, fz_obj *rdb, fz_stream *file, char *buf)
{
	fz_error error;
	int key;

	key = buf[0];
	if (buf[1])
	{
		key |= buf[1] << 8;
		if (buf[2])
		{
			key |= buf[2] << 16;
			if (buf[3])
				key = 0;
		}
	}

	switch (key)
	{
	case A('"'): pdf_run_dquote(csi); break;
	case A('\''): pdf_run_squote(csi); break;
//	case A('B'): pdf_run_B(csi); break;
//	case B('B','*'): pdf_run_Bstar(csi); break;
//	case C('B','D','C'): pdf_run_BDC(csi, rdb); break; /* SumatraPDF: support inline OCGs */
//	case B('B','I'):
//		error = pdf_run_BI(csi, rdb, file);
//		if (error)
//			return fz_rethrow(error, "cannot draw inline image");
//		break;
	case C('B','M','C'): pdf_run_BMC(csi); break;
	case B('B','T'): pdf_run_BT(csi); break;
	case B('B','X'): pdf_run_BX(csi); break;
//	case B('C','S'): pdf_run_CS(csi, rdb); break;
	case B('D','P'): pdf_run_DP(csi); break;
//	case B('D','o'):
//		error = pdf_run_Do(csi, rdb);
//		if (error)
//			fz_catch(error, "cannot draw xobject/image");
//		break;
//	case C('E','M','C'): pdf_run_EMC(csi); break;
	case B('E','T'): pdf_run_ET(csi); break;
	case B('E','X'): pdf_run_EX(csi); break;
//	case A('F'): pdf_run_F(csi); break;
//	case A('G'): pdf_run_G(csi); break;
	case A('J'): pdf_run_J(csi); break;
//	case A('K'): pdf_run_K(csi); break;
	case A('M'): pdf_run_M(csi); break;
	case B('M','P'): pdf_run_MP(csi); break;
	case A('Q'): pdf_run_Q(csi); break;
//	case B('R','G'): pdf_run_RG(csi); break;
//	case A('S'): pdf_run_S(csi); break;
//	case B('S','C'): pdf_run_SC(csi, rdb); break;
//	case C('S','C','N'): pdf_run_SC(csi, rdb); break;
	case B('T','*'): pdf_run_Tstar(csi); break;
	case B('T','D'): pdf_run_TD(csi); break;
	case B('T','J'): pdf_run_TJ(csi); break;
	case B('T','L'): pdf_run_TL(csi); break;
	case B('T','c'): pdf_run_Tc(csi); break;
	case B('T','d'): pdf_run_Td(csi); break;
	case B('T','f'):
		error = pdf_run_Tf(csi, rdb);
		if (error)
			fz_catch(error, "cannot set font");
		break;
	case B('T','j'): pdf_run_Tj(csi); break;
	case B('T','m'): pdf_run_Tm(csi); break;
	case B('T','r'): pdf_run_Tr(csi); break;
	case B('T','s'): pdf_run_Ts(csi); break;
	case B('T','w'): pdf_run_Tw(csi); break;
	case B('T','z'): pdf_run_Tz(csi); break;
	case A('W'): pdf_run_W(csi); break;
	case B('W','*'): pdf_run_Wstar(csi); break;
//	case A('b'): pdf_run_b(csi); break;
//	case B('b','*'): pdf_run_bstar(csi); break;
	case A('c'): pdf_run_c(csi); break;
	case B('c','m'): pdf_run_cm(csi); break;
//	case B('c','s'): pdf_run_cs(csi, rdb); break;
//	case A('d'): pdf_run_d(csi); break;
	case B('d','0'): pdf_run_d0(csi); break;
	case B('d','1'): pdf_run_d1(csi); break;
//	case A('f'): pdf_run_f(csi); break;
//	case B('f','*'): pdf_run_fstar(csi); break;
//	case A('g'): pdf_run_g(csi); break;
	case B('g','s'):
		error = pdf_run_gs(csi, rdb);
		if (error)
			fz_catch(error, "cannot set graphics state");
		break;
	case A('h'): pdf_run_h(csi); break;
	case A('i'): pdf_run_i(csi); break;
	case A('j'): pdf_run_j(csi); break;
//	case A('k'): pdf_run_k(csi); break;
	case A('l'): pdf_run_l(csi); break;
	case A('m'): pdf_run_m(csi); break;
//	case A('n'): pdf_run_n(csi); break;
	case A('q'): pdf_run_q(csi); break;
	case B('r','e'): pdf_run_re(csi); break;
//	case B('r','g'): pdf_run_rg(csi); break;
	case B('r','i'): pdf_run_ri(csi); break;
//	case A('s'): pdf_run(csi); break;
//	case B('s','c'): pdf_run_sc(csi, rdb); break;
//	case C('s','c','n'): pdf_run_sc(csi, rdb); break;
//	case B('s','h'):
//		error = pdf_run_sh(csi, rdb);
//		if (error)
//			fz_catch(error, "cannot draw shading");
//		break;
	case A('v'): pdf_run_v(csi); break;
	case A('w'): pdf_run_w(csi); break;
	case A('y'): pdf_run_y(csi); break;
	default:
		break;
	}

	return fz_okay;
}

static fz_error
pdf_run_stream(pdf_csi *csi, fz_obj *rdb, fz_stream *file, char *buf, int buflen)
{
	fz_error error;
	int tok, len, in_array;

	/* make sure we have a clean slate if we come here from flush_text */
	pdf_clear_stack(csi);
	in_array = 0;

	while (1)
	{
		if (csi->top == nelem(csi->stack) - 1)
			return fz_throw("stack overflow");

		error = pdf_lex(&tok, file, buf, buflen, &len);
		if (error)
			return fz_rethrow(error, "lexical error in content stream");

		if (in_array)
		{
			if (tok == PDF_TOK_CLOSE_ARRAY)
			{
				in_array = 0;
			}
			else if (tok == PDF_TOK_INT || tok == PDF_TOK_REAL)
			{
				pdf_gstate *gstate = csi->gstate + csi->gtop;
				pdf_show_space(csi, -fz_atof(buf) * gstate->size * 0.001f);
			}
			else if (tok == PDF_TOK_STRING)
			{
				pdf_show_string(csi, (unsigned char *)buf, len);
			}
			else if (tok == PDF_TOK_KEYWORD)
			{
				if (!strcmp(buf, "Tw") || !strcmp(buf, "Tc"))
					fz_warn("ignoring keyword '%s' inside array", buf);
				else
					return fz_throw("syntax error in array");
			}
			else if (tok == PDF_TOK_EOF)
				return fz_okay;
			else
				return fz_throw("syntax error in array");
		}

		else switch (tok)
		{
		case PDF_TOK_ENDSTREAM:
		case PDF_TOK_EOF:
			return fz_okay;

		case PDF_TOK_OPEN_ARRAY:
			if (!csi->in_text)
			{
				error = pdf_parse_array(&csi->obj, csi->xref, file, buf, buflen);
				if (error)
					return fz_rethrow(error, "cannot parse array");
			}
			else
			{
				in_array = 1;
			}
			break;

		case PDF_TOK_OPEN_DICT:
			error = pdf_parse_dict(&csi->obj, csi->xref, file, buf, buflen);
			if (error)
				return fz_rethrow(error, "cannot parse dictionary");
			break;

		case PDF_TOK_NAME:
			fz_strlcpy(csi->name, buf, sizeof(csi->name));
			break;

		case PDF_TOK_INT:
			csi->stack[csi->top] = atoi(buf);
			csi->top ++;
			break;

		case PDF_TOK_REAL:
			csi->stack[csi->top] = fz_atof(buf);
			csi->top ++;
			break;

		case PDF_TOK_STRING:
			if (len <= sizeof(csi->string))
			{
				memcpy(csi->string, buf, len);
				csi->string_len = len;
			}
			else
			{
				csi->obj = fz_new_string(buf, len);
			}
			break;

		case PDF_TOK_KEYWORD:
			error = pdf_run_keyword(csi, rdb, file, buf);
			if (error)
				return fz_rethrow(error, "cannot run keyword");
			pdf_clear_stack(csi);
			break;

		default:
			return fz_throw("syntax error in content stream");
		}
	}
}

/*
 * Entry points
 */

static fz_error
pdf_run_buffer(pdf_csi *csi, fz_obj *rdb, fz_buffer *contents)
{
	/* SumatraPDF: be slightly more defensive */
	if (contents)
	{
	fz_error error;
	int len = sizeof csi->xref->scratch;
	char *buf = (char *) fz_malloc(len); /* we must be re-entrant for type3 fonts */
	fz_stream *file = fz_open_buffer(contents);
	int save_in_text = csi->in_text;
	csi->in_text = 0;
	error = pdf_run_stream(csi, rdb, file, buf, len);
	csi->in_text = save_in_text;
	fz_close(file);
	fz_free(buf);
	if (error)
		/* cf. http://bugs.ghostscript.com/show_bug.cgi?id=692260 */
		fz_catch(error, "couldn't parse the whole content stream, rendering anyway");
	return fz_okay;
	/* SumatraPDF: be slightly more defensive */
	}
	return fz_throw("cannot run NULL content stream");
}

fz_error
pdf_run_page_with_usage(pdf_xref *xref, pdf_page *page, fz_device *dev, fz_matrix ctm, char *target)
{
	pdf_csi *csi;
	fz_error error;

	if (page->transparency)
		fz_begin_group(dev, fz_transform_rect(ctm, page->mediabox), 1, 0, 0, 1);

	csi = pdf_new_csi(xref, dev, ctm, target);
	error = pdf_run_buffer(csi, page->resources, page->contents);
	pdf_free_csi(csi);
	if (error)
		return fz_rethrow(error, "cannot parse page content stream");

	if (page->transparency)
		fz_end_group(dev);

	return fz_okay;
}

#endif