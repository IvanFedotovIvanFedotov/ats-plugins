#ifndef ERROR_H
#define ERROR_H

#include <gst/gst.h>
#include <glib.h>

typedef enum { SILENCE, LOUDNESS, PARAM_NUMBER } PARAMETER;

const char* param_to_string (PARAMETER);
gboolean param_upper_boundary (PARAMETER p);

typedef struct {
        gboolean cont_en;
        gfloat   cont;
        gboolean peak_en;
        gfloat   peak;
        gfloat   duration;
} BOUNDARY;

typedef struct {
        gboolean cont;
        gboolean peak;
        gint64   time;
} ErrFlags;

/*                  flags    boundaries  timestamp uppre_bound? duration duration_d value    */
void err_flags_cmp (ErrFlags*, BOUNDARY*, gint64, gboolean, float*, float, float);

typedef struct {
        guint     period;
        guint     current;
        ErrFlags* err_flags;
} Errors;

Errors*  errors_new(guint);
void     errors_reset(Errors*);
void     errors_delete(Errors*);
gint     errors_append(Errors*, ErrFlags[PARAM_NUMBER]);
gboolean errors_is_full(Errors*);
gpointer errors_dump(Errors*, gsize*);

#endif /* ERROR_H */
