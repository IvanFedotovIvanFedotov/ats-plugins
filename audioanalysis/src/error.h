#ifndef ERROR_H
#define ERROR_H

#include <gst/gst.h>
#include <glib.h>
#include "audiodata.h"

typedef enum { SILENCE_SHORTT, LOUDNESS_SHORTT, SILENCE_MOMENT, LOUDNESS_MOMENT, PARAM_NUMBER } PARAMETER;

const char* param_to_string (PARAMETER);
gboolean param_upper_boundary (PARAMETER p);

typedef struct {
        guint32  counter;
        guint32  size;
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

void err_reset (Error*, guint32);
void err_add_timestamp (Error e [PARAM_NUMBER], gint64 t);
void err_add_params (Error e [PARAM_NUMBER], AudioParams*);
gpointer err_dump (Error e [PARAM_NUMBER]);

/*                  flags    boundaries  uppre_bound? duration duration_d value    */
void err_flags_cmp (Error*, BOUNDARY*, gboolean, float*, float, double);

#endif /* ERROR_H */
