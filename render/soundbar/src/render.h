#include "gstsoundbar.h"

inline void render (struct state * state,
                    struct video_info * vi,
                    struct audio_info * ai,
                    guint32 * vdata,
                    gint16 * adata) {

        int i;
        guint32 fing = adata[0] + (((guint32) adata[1]) << 16);

        for (i = 0; i < vi->height * vi->width; i++) {
                vdata[i] = fing;
        }
}
