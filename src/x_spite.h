/* PgD4S (plugdata for Spite): objects for converted Max for Live devices.
   Their state is read by plugdata, which draws them; see x_spite.c. */

#ifndef X_SPITE_H
#define X_SPITE_H

#include "m_pd.h"
#include "g_canvas.h"

    /* [spite.tab]: live.tab, a grid of cells with text or pictures, one selected */
typedef struct _spitetab
{
    t_object x_obj;
    t_glist *x_glist;
    int x_w, x_h;               /* size in pixels */
    int x_rows, x_cols;         /* grid; 0 for one of them: as many as needed */
    int x_spacing;              /* gap between cells, pixels */
    int x_round;                /* corner radius, pixels */
    int x_fontsize;             /* text size, pixels */
    t_symbol *x_bg, *x_fg;      /* cell and text/picture colours */
    t_symbol *x_onbg, *x_onfg;  /* the same for the selected cell */
    t_symbol *x_snd_raw, *x_rcv_raw;    /* as typed ($0 unexpanded) */
    t_symbol *x_snd, *x_rcv;            /* expanded, or 0 */
    int x_init;                 /* index selected at creation */
    int x_loadinit;             /* output it on load */
    int x_n;                    /* number of items */
    t_symbol **x_items;         /* their texts */
    t_symbol **x_pics;          /* their picture files, &s_ for none */
    int x_sel;                  /* the selected index */
    t_outlet *x_out_index;
    t_outlet *x_out_name;
} t_spitetab;

    /* cells as laid out by plugdata: rows and columns after "0 = as needed" */
EXTERN void spitetab_grid(t_spitetab *x, int *rows, int *cols);
    /* select an index (clamped) and output it, as a click in the GUI does */
EXTERN void spitetab_float(t_spitetab *x, t_floatarg f);

#endif
