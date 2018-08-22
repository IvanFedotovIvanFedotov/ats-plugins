#include "error.h"
#include <malloc.h>
#include <string.h>

void
err_flags_cmp (Error* err, BOUNDARY* bounds, gboolean upper, float* dur, float dur_d, float val)
{
        gboolean peak = FALSE, cont = FALSE;
        if (bounds->peak_en) {
                peak = upper ? (val > bounds->peak) : (val < bounds->peak);
        }
        if (bounds->cont_en) {
                gboolean b = upper ? (val > bounds->cont) : (val < bounds->cont);
                if (b) {
                        *dur += dur_d;
                } else {
                        *dur = 0.0;
                }
                cont = *dur > bounds->duration;
        }
        if (peak) {
                err->counter++;
                err->peak_flag = peak;
        }

        if (cont) {
                err->cont_flag = cont;
        }
}

void
err_reset (Error e [PARAM_NUMBER], guint sz) {
        for (int i = 0; i < PARAM_NUMBER; i++) {
                e[i].counter = 0;
                e[i].size = sz;
                e[i].peak_flag = FALSE;
                e[i].cont_flag = FALSE;
        }
}

void
err_add_timestamp (Error e [PARAM_NUMBER], gint64 t) {
        for (int i = 0; i < PARAM_NUMBER; i++) {
                e[i].timestamp = t;
        }
}

void
err_add_params (Error e [PARAM_NUMBER], VideoParams* p) {
        for (int i = 0; i < PARAM_NUMBER; i++) {
                e[i].params = *param_of_video_params(p, i);
        }
}

gpointer
err_dump (Error e [PARAM_NUMBER]) {
        Error* buf = (Error*)malloc(sizeof(Error) * PARAM_NUMBER);
        memcpy(buf, e, sizeof(Error) * PARAM_NUMBER);
        return buf;
}
