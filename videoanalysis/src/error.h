#ifndef ERROR_H
#define ERROR_H

#include <gst/gst.h>
#include <glib.h>
#include "videodata.h"

typedef struct {
        guint    counter;
        guint    size;
        Param    params;
        gint64   timestamp;
        gboolean peak_flag;
        gboolean cont_flag;
} Error;

typedef struct {
        gboolean cont_en;
        gfloat   cont;
        gboolean peak_en;
        gfloat   peak;
        gfloat   duration;
} BOUNDARY;

void err_reset (Error*, guint);
void err_add_timestamp (Error e [PARAM_NUMBER], gint64 t);
void err_add_params (Error e [PARAM_NUMBER], VideoParams*);
gpointer err_dump (Error e [PARAM_NUMBER]);

/*                  flags    boundaries upper_bound? duration duration_d value    */
void err_flags_cmp (Error*, BOUNDARY*, gboolean, float*, float, float);

#endif /* BOUNDARY_H */
