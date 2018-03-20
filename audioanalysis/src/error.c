#include "error.h"
#include <malloc.h>
#include <string.h>

const char*
param_to_string (PARAMETER p)
{
        switch (p) {
        case SILENCE_MOMENT:
        case SILENCE_SHORTT:
                return "silence";
                break;
        case LOUDNESS_MOMENT:
        case LOUDNESS_SHORTT:
                return "loudness";
                break;
        default:
                return "unknown";
                break;
        }
}

gboolean
param_upper_boundary (PARAMETER p)
{
        switch (p) {
        case SILENCE_MOMENT:
        case SILENCE_SHORTT:
                return FALSE;
                break;
        default:
                return TRUE;
                break;
        }
}

void
err_flags_cmp (Error* err, BOUNDARY* bounds, gboolean upper, float* dur, float dur_d, double val)
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
err_reset (Error* e, guint32 sz) {
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
err_add_params (Error e [PARAM_NUMBER], AudioParams* p) {
        for (int i = 0; i < PARAM_NUMBER; i++) {
                switch (i) {
                case LOUDNESS_SHORTT:
                case SILENCE_SHORTT:
                        e[i].params = p->shortt;
                        break;
                case LOUDNESS_MOMENT:
                case SILENCE_MOMENT:
                        e[i].params = p->moment;
                        break;
                default: break;
                }
        }
}

gpointer
err_dump (Error e [PARAM_NUMBER]) {
        Error* buf = (Error*)malloc(sizeof(Error) * PARAM_NUMBER);
        memcpy(buf, e, sizeof(Error) * PARAM_NUMBER);
        return buf;
}
