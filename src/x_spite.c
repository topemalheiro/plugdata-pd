/* PgD4S (plugdata for Spite): objects for converted Max for Live devices.

   [spite.tab]     live.tab: a grid of cells with text or pictures, one selected.
                   plugdata draws it and turns clicks into "float <index>".
   [pcontrol]      Max's pcontrol: "open"/"close" the subpatchers its outlet is
                   wired to.
   [thispatcher]   Max's thispatcher (the window messages): size, flags, exec,
                   getsize, front, wclose, presentation, offset.

   A canvas marked "#X presentation open 1" (Max's openinpresentation) is opened
   by plugdata's own [pcontrol]/[thispatcher] GUIs as a window showing its
   presentation, so these leave it alone. */

#include <string.h>
#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"
#include "x_spite.h"

/* ----------------------------- spite.tab ------------------------------- */

static t_class *spitetab_class;
static t_widgetbehavior spitetab_widgetbehavior;

void spitetab_grid(t_spitetab *x, int *rows, int *cols)
{
    int r = x->x_rows, c = x->x_cols, n = x->x_n > 0 ? x->x_n : 1;
    if (r <= 0 && c <= 0)
        r = 1;
    if (r <= 0)
        r = (n + c - 1) / c;
    if (c <= 0)
        c = (n + r - 1) / r;
    *rows = r;
    *cols = c;
}

static t_symbol *spitetab_atomsymbol(t_atom *a)
{
    char buf[MAXPDSTRING];
    if (a->a_type == A_SYMBOL)
        return (a->a_w.w_symbol);
    atom_string(a, buf, MAXPDSTRING);
    return (gensym(buf));
}

static void spitetab_freeitems(t_spitetab *x)
{
    if (x->x_items)
        freebytes(x->x_items, x->x_n * sizeof(t_symbol *));
    if (x->x_pics)
        freebytes(x->x_pics, x->x_n * sizeof(t_symbol *));
    x->x_items = x->x_pics = 0;
    x->x_n = 0;
}

    /* new items; each cell keeps its picture */
static void spitetab_setitems(t_spitetab *x, int n, t_atom *items)
{
    int i, oldn = x->x_n;
    t_symbol **oldpics = x->x_pics;
    if (x->x_items)
        freebytes(x->x_items, oldn * sizeof(t_symbol *));
    if (n < 1)
        n = 1;
    x->x_n = n;
    x->x_items = (t_symbol **)getbytes(n * sizeof(t_symbol *));
    x->x_pics = (t_symbol **)getbytes(n * sizeof(t_symbol *));
    for (i = 0; i < n; i++)
    {
        x->x_items[i] = items ? spitetab_atomsymbol(items + i) : &s_;
        x->x_pics[i] = (oldpics && i < oldn) ? oldpics[i] : &s_;
    }
    if (oldpics)
        freebytes(oldpics, oldn * sizeof(t_symbol *));
    if (x->x_sel >= n)
        x->x_sel = n - 1;
}

static void spitetab_output(t_spitetab *x)
{
    t_symbol *name = x->x_items[x->x_sel];
    outlet_symbol(x->x_out_name, name);
    outlet_float(x->x_out_index, x->x_sel);
    if (x->x_snd && x->x_snd->s_thing)
        pd_float(x->x_snd->s_thing, x->x_sel);
}

static int spitetab_clamp(t_spitetab *x, t_floatarg f)
{
    int i = (int)f;
    return (i < 0 ? 0 : i >= x->x_n ? x->x_n - 1 : i);
}

void spitetab_float(t_spitetab *x, t_floatarg f)
{
    x->x_sel = spitetab_clamp(x, f);
    spitetab_output(x);
}

static void spitetab_bang(t_spitetab *x)
{
    spitetab_output(x);
}

static void spitetab_set(t_spitetab *x, t_floatarg f)
{
    x->x_sel = spitetab_clamp(x, f);
}

static void spitetab_items(t_spitetab *x, t_symbol *s, int argc, t_atom *argv)
{
    spitetab_setitems(x, argc, argc ? argv : 0);
}

static void spitetab_pictures(t_spitetab *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    for (i = 0; i < x->x_n; i++)
    {
        t_symbol *pic = i < argc ? spitetab_atomsymbol(argv + i) : &s_;
        x->x_pics[i] = (pic == gensym("-") ? &s_ : pic);
    }
}

static void spitetab_grid_method(t_spitetab *x, t_floatarg rows, t_floatarg cols)
{
    x->x_rows = rows < 0 ? 0 : (int)rows;
    x->x_cols = cols < 0 ? 0 : (int)cols;
}

static void spitetab_spacing(t_spitetab *x, t_floatarg f)
{
    x->x_spacing = f < 0 ? 0 : (int)f;
}

static void spitetab_round(t_spitetab *x, t_floatarg f)
{
    x->x_round = f < 0 ? 0 : (int)f;
}

static void spitetab_fontsize(t_spitetab *x, t_floatarg f)
{
    x->x_fontsize = f < 4 ? 4 : (int)f;
}

static void spitetab_colors(t_spitetab *x, t_symbol *bg, t_symbol *fg,
    t_symbol *onbg, t_symbol *onfg)
{
    if (*bg->s_name) x->x_bg = bg;
    if (*fg->s_name) x->x_fg = fg;
    if (*onbg->s_name) x->x_onbg = onbg;
    if (*onfg->s_name) x->x_onfg = onfg;
}

static void spitetab_dim(t_spitetab *x, t_floatarg w, t_floatarg h)
{
    x->x_w = w < 8 ? 8 : (int)w;
    x->x_h = h < 8 ? 8 : (int)h;
    canvas_fixlinesfor(x->x_glist, &x->x_obj);
}

static t_symbol *spitetab_expand(t_spitetab *x, t_symbol *raw)
{
    if (!raw || raw == &s_ || raw == gensym("empty"))
        return (0);
    return (canvas_realizedollar(x->x_glist, raw));
}

static void spitetab_send(t_spitetab *x, t_symbol *s)
{
    x->x_snd_raw = (s == &s_ ? gensym("empty") : s);
    x->x_snd = spitetab_expand(x, x->x_snd_raw);
}

static void spitetab_receive(t_spitetab *x, t_symbol *s)
{
    if (x->x_rcv)
        pd_unbind(&x->x_obj.ob_pd, x->x_rcv);
    x->x_rcv_raw = (s == &s_ ? gensym("empty") : s);
    if ((x->x_rcv = spitetab_expand(x, x->x_rcv_raw)))
        pd_bind(&x->x_obj.ob_pd, x->x_rcv);
}

static void spitetab_loadbang(t_spitetab *x, t_floatarg action)
{
    if ((int)action == LB_LOAD && x->x_loadinit)
        spitetab_float(x, x->x_init);
}

static void spitetab_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_spitetab *x = (t_spitetab *)z;
    *xp1 = text_xpix(&x->x_obj, glist);
    *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = *xp1 + x->x_w * glist->gl_zoom;
    *yp2 = *yp1 + x->x_h * glist->gl_zoom;
}

static void spitetab_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_spitetab *x = (t_spitetab *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    canvas_fixlinesfor(glist, &x->x_obj);
}

static void spitetab_select(t_gobj *z, t_glist *glist, int state)
{
}

static void spitetab_activate(t_gobj *z, t_glist *glist, int state)
{
}

static void spitetab_delete(t_gobj *z, t_glist *glist)
{
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void spitetab_vis(t_gobj *z, t_glist *glist, int vis)
{
}

static void spitetab_save(t_gobj *z, t_binbuf *b)
{
    t_spitetab *x = (t_spitetab *)z;
    int i, havepics = 0;
    binbuf_addv(b, "ssii", gensym("#X"), gensym("obj"),
        (int)x->x_obj.te_xpix, (int)x->x_obj.te_ypix);
    binbuf_addv(b, "siiiiiiissssssiii", gensym("spite.tab"),
        x->x_w, x->x_h, x->x_rows, x->x_cols, x->x_spacing, x->x_round,
        x->x_fontsize, x->x_bg, x->x_fg, x->x_onbg, x->x_onfg,
        x->x_snd_raw, x->x_rcv_raw, x->x_init, x->x_loadinit, x->x_n);
    for (i = 0; i < x->x_n; i++)
    {
        binbuf_addv(b, "s", x->x_items[i]);
        if (x->x_pics[i] != &s_)
            havepics = 1;
    }
    if (havepics)
        for (i = 0; i < x->x_n; i++)
            binbuf_addv(b, "s", x->x_pics[i] == &s_ ? gensym("-") : x->x_pics[i]);
    binbuf_addv(b, ";");
}

    /* spite.tab <w> <h> <rows> <cols> <spacing> <round> <fontsize> <bg> <fg>
       <onbg> <onfg> <snd> <rcv> <init> <loadinit> <n> <items...> [<pics...>] */
static void *spitetab_new(t_symbol *s, int argc, t_atom *argv)
{
    t_spitetab *x = (t_spitetab *)pd_new(spitetab_class);
    t_atom defaults[2];
    int n;
    x->x_glist = canvas_getcurrent();
    x->x_w = 120; x->x_h = 20;
    x->x_rows = 1; x->x_cols = 0;
    x->x_spacing = 2; x->x_round = 3; x->x_fontsize = 10;
    x->x_bg = gensym("#262626"); x->x_fg = gensym("#c0c0c0");
    x->x_onbg = gensym("#7d7d7d"); x->x_onfg = gensym("#ffffff");
    x->x_snd_raw = x->x_rcv_raw = gensym("empty");
    x->x_init = x->x_loadinit = 0;
    x->x_sel = 0;
    if (argc >= 16)
    {
        x->x_w = atom_getintarg(0, argc, argv);
        x->x_h = atom_getintarg(1, argc, argv);
        x->x_rows = atom_getintarg(2, argc, argv);
        x->x_cols = atom_getintarg(3, argc, argv);
        x->x_spacing = atom_getintarg(4, argc, argv);
        x->x_round = atom_getintarg(5, argc, argv);
        x->x_fontsize = atom_getintarg(6, argc, argv);
        if (argv[7].a_type == A_SYMBOL) x->x_bg = argv[7].a_w.w_symbol;
        if (argv[8].a_type == A_SYMBOL) x->x_fg = argv[8].a_w.w_symbol;
        if (argv[9].a_type == A_SYMBOL) x->x_onbg = argv[9].a_w.w_symbol;
        if (argv[10].a_type == A_SYMBOL) x->x_onfg = argv[10].a_w.w_symbol;
        x->x_snd_raw = spitetab_atomsymbol(argv + 11);
        x->x_rcv_raw = spitetab_atomsymbol(argv + 12);
        x->x_init = atom_getintarg(13, argc, argv);
        x->x_loadinit = atom_getintarg(14, argc, argv);
        n = atom_getintarg(15, argc, argv);
        if (n > argc - 16)
            n = argc - 16;
        if (n > 0)
        {
            spitetab_setitems(x, n, argv + 16);
            if (argc >= 16 + 2 * n)
                spitetab_pictures(x, 0, n, argv + 16 + n);
        }
    }
    if (!x->x_n)
    {
        SETSYMBOL(defaults, gensym("one"));
        SETSYMBOL(defaults + 1, gensym("two"));
        spitetab_setitems(x, 2, defaults);
    }
    if (x->x_w < 8) x->x_w = 8;
    if (x->x_h < 8) x->x_h = 8;
    if (x->x_fontsize < 4) x->x_fontsize = 4;
    x->x_sel = spitetab_clamp(x, x->x_init);
    x->x_snd = spitetab_expand(x, x->x_snd_raw);
    if ((x->x_rcv = spitetab_expand(x, x->x_rcv_raw)))
        pd_bind(&x->x_obj.ob_pd, x->x_rcv);
    x->x_out_index = outlet_new(&x->x_obj, &s_float);
    x->x_out_name = outlet_new(&x->x_obj, &s_symbol);
    return (x);
}

static void spitetab_free(t_spitetab *x)
{
    if (x->x_rcv)
        pd_unbind(&x->x_obj.ob_pd, x->x_rcv);
    spitetab_freeitems(x);
}

/* ----------------------------- pcontrol -------------------------------- */

static t_class *pcontrol_class;

typedef struct _pcontrol
{
    t_object x_obj;
} t_pcontrol;

    /* open or close every canvas wired to the outlet; one marked "open in
       presentation" is opened by plugdata's [pcontrol] GUI instead */
static void pcontrol_vis(t_pcontrol *x, int vis)
{
    t_outlet *out = 0;
    t_outconnect *oc = obj_starttraverseoutlet(&x->x_obj, &out, 0);
    while (oc)
    {
        t_object *dest = 0;
        t_inlet *in = 0;
        int which = 0;
        oc = obj_nexttraverseoutlet(oc, &dest, &in, &which);
        if (dest && pd_class(&dest->te_pd) == canvas_class)
        {
            t_canvas *c = (t_canvas *)dest;
            if (vis && c->gl_presentation_open)
                continue;
            canvas_vis(c, vis);
        }
    }
}

static void pcontrol_open(t_pcontrol *x)
{
    pcontrol_vis(x, 1);
}

static void pcontrol_close(t_pcontrol *x)
{
    pcontrol_vis(x, 0);
}

static void pcontrol_anything(t_pcontrol *x, t_symbol *s, int argc, t_atom *argv)
{
    /* Max's enable/disable/load... have no meaning here */
}

static void *pcontrol_new(void)
{
    t_pcontrol *x = (t_pcontrol *)pd_new(pcontrol_class);
    outlet_new(&x->x_obj, 0);
    return (x);
}

/* ---------------------------- thispatcher ------------------------------ */

static t_class *thispatcher_class;

typedef struct _thispatcher
{
    t_object x_obj;
    t_canvas *x_canvas;
    int x_size[4];
    int x_havesize;
    t_outlet *x_out;
} t_thispatcher;

static void thispatcher_window(t_thispatcher *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *what = atom_getsymbolarg(0, argc, argv);
    t_canvas *c = x->x_canvas;
    if (what == gensym("size") && argc >= 5)
    {
        int i;
        for (i = 0; i < 4; i++)
            x->x_size[i] = atom_getintarg(i + 1, argc, argv);
        x->x_havesize = 1;
    }
    else if (what == gensym("flags"))
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            t_symbol *flag = atom_getsymbolarg(i, argc, argv);
            if (flag == gensym("float"))
                c->gl_presentation_float = 1;
            else if (flag == gensym("nofloat"))
                c->gl_presentation_float = 0;
        }
    }
    else if (what == gensym("exec"))
    {
        if (x->x_havesize && x->x_size[2] > x->x_size[0] && x->x_size[3] > x->x_size[1])
        {
            c->gl_screenx1 = x->x_size[0];
            c->gl_screeny1 = x->x_size[1];
            c->gl_screenx2 = x->x_size[2];
            c->gl_screeny2 = x->x_size[3];
                /* a subpatch shown in its own presentation window takes the
                size too (plugdata sizes that window by it) */
            if (c->gl_presentation_open)
                canvas_presentation_setsize(c, x->x_size[2] - x->x_size[0],
                    x->x_size[3] - x->x_size[1]);
        }
        x->x_havesize = 0;
    }
    else if (what == gensym("getsize"))
    {
        t_atom at[5];
        SETSYMBOL(at, gensym("size"));
        SETFLOAT(at + 1, c->gl_screenx1);
        SETFLOAT(at + 2, c->gl_screeny1);
        SETFLOAT(at + 3, c->gl_screenx2);
        SETFLOAT(at + 4, c->gl_screeny2);
        outlet_anything(x->x_out, gensym("window"), 5, at);
    }
}

static void thispatcher_front(t_thispatcher *x)
{
    if (!x->x_canvas->gl_presentation_open)
        canvas_vis(x->x_canvas, 1);
}

static void thispatcher_wclose(t_thispatcher *x)
{
    canvas_vis(x->x_canvas, 0);
}

static void thispatcher_presentation(t_thispatcher *x, t_floatarg f)
{
    x->x_canvas->gl_presentation_open = (f != 0);
}

    /* a bpatcher's offset: show the content shifted, through the
       graph-on-parent margins */
static void thispatcher_offset(t_thispatcher *x, t_floatarg dx, t_floatarg dy)
{
    t_canvas *c = x->x_canvas;
    t_atom at[9];
    SETFLOAT(at, c->gl_x1);
    SETFLOAT(at + 1, c->gl_y1);
    SETFLOAT(at + 2, c->gl_x2);
    SETFLOAT(at + 3, c->gl_y2);
    SETFLOAT(at + 4, c->gl_pixwidth);
    SETFLOAT(at + 5, c->gl_pixheight);
    SETFLOAT(at + 6, c->gl_isgraph + 2 * c->gl_hidetext);
    SETFLOAT(at + 7, -dx);
    SETFLOAT(at + 8, -dy);
    pd_typedmess(&c->gl_pd, gensym("coords"), 9, at);
}

static void thispatcher_anything(t_thispatcher *x, t_symbol *s, int argc, t_atom *argv)
{
    /* the rest of Max's thispatcher (scripting, path...) isn't supported */
}

static void *thispatcher_new(void)
{
    t_thispatcher *x = (t_thispatcher *)pd_new(thispatcher_class);
    x->x_canvas = canvas_getcurrent();
    x->x_havesize = 0;
    x->x_out = outlet_new(&x->x_obj, 0);
    outlet_new(&x->x_obj, 0);
    return (x);
}

/* -------------------------------- setup --------------------------------- */

void set_plugdata_object_probe_enabled(int);

void x_spite_setup(void)
{
        /* plugdata hears these objects' messages, to redraw them or open
        presentation windows */
    set_plugdata_object_probe_enabled(1);

    spitetab_class = class_new(gensym("spite.tab"), (t_newmethod)spitetab_new,
        (t_method)spitetab_free, sizeof(t_spitetab), 0, A_GIMME, 0);
    class_addfloat(spitetab_class, spitetab_float);
    class_addbang(spitetab_class, spitetab_bang);
    class_addmethod(spitetab_class, (t_method)spitetab_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_items, gensym("items"), A_GIMME, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_pictures, gensym("pictures"), A_GIMME, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_grid_method, gensym("grid"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_spacing, gensym("spacing"), A_FLOAT, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_round, gensym("round"), A_FLOAT, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_fontsize, gensym("fontsize"), A_FLOAT, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_colors, gensym("colors"),
        A_DEFSYM, A_DEFSYM, A_DEFSYM, A_DEFSYM, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_dim, gensym("dim"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(spitetab_class, (t_method)spitetab_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
    spitetab_widgetbehavior.w_getrectfn = spitetab_getrect;
    spitetab_widgetbehavior.w_displacefn = spitetab_displace;
    spitetab_widgetbehavior.w_selectfn = spitetab_select;
    spitetab_widgetbehavior.w_activatefn = spitetab_activate;
    spitetab_widgetbehavior.w_deletefn = spitetab_delete;
    spitetab_widgetbehavior.w_visfn = spitetab_vis;
    spitetab_widgetbehavior.w_clickfn = 0;
    class_setwidget(spitetab_class, &spitetab_widgetbehavior);
    class_setsavefn(spitetab_class, spitetab_save);

    pcontrol_class = class_new(gensym("pcontrol"), (t_newmethod)pcontrol_new,
        0, sizeof(t_pcontrol), 0, 0);
    class_addmethod(pcontrol_class, (t_method)pcontrol_open, gensym("open"), 0);
    class_addmethod(pcontrol_class, (t_method)pcontrol_close, gensym("close"), 0);
    class_addanything(pcontrol_class, pcontrol_anything);

    thispatcher_class = class_new(gensym("thispatcher"), (t_newmethod)thispatcher_new,
        0, sizeof(t_thispatcher), 0, 0);
    class_addmethod(thispatcher_class, (t_method)thispatcher_window, gensym("window"), A_GIMME, 0);
    class_addmethod(thispatcher_class, (t_method)thispatcher_front, gensym("front"), 0);
    class_addmethod(thispatcher_class, (t_method)thispatcher_wclose, gensym("wclose"), 0);
    class_addmethod(thispatcher_class, (t_method)thispatcher_presentation, gensym("presentation"), A_FLOAT, 0);
    class_addmethod(thispatcher_class, (t_method)thispatcher_offset, gensym("offset"), A_FLOAT, A_FLOAT, 0);
    class_addanything(thispatcher_class, thispatcher_anything);

    set_plugdata_object_probe_enabled(0);
}
