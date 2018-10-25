#include "gstsoundbar.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
// BGRA ABGR
static const guint32 red     = 0xff0000ff;
static const guint32 orange  = 0xff00a5ff;
static const guint32 yellow  = 0xff00ffff;
static const guint32 l_green = 0xff00ff00;
static const guint32 green   = 0xff008000;
static const guint32 white   = 0xffffffff;    /* 0xNNNNNNNN is 0xBBGGRRAA */
static const guint32 black   = 0xff000000;
static const guint32 transparent_white = 0x00ffffff;
static const guint32 transparent_black = 0x00000000;

static const guint8 lvl_height = 5;  /* soundbar level height in px */

static inline gint sample_to_db (gint sample) {
        if (sample < 310) {
                return -40;
        }
        else if (sample < 347) {
                return -39;
        }
        else if (sample < 390) {
                return -38;
        }
        else if (sample < 437) {
                return -37;
        }
        else if (sample < 491) {
                return -36;
        }
        else if (sample < 550) {
                return -35;
        }
        else if (sample < 618) {
                return -34;
        }
        else if (sample < 693) {
                return -33;
        }
        else if (sample < 777) {
                return -32;
        }
        else if (sample < 872) {
                return -31;
        }
        else if (sample < 979) {
                return -30;
        }
        else if (sample < 1098) {
                return -29;
        }
        else if (sample < 1232) {
                return -28;
        }
        else if (sample < 1383) {
                return -27;
        }
        else if (sample < 1551) {
                return -26;
        }
        else if (sample < 1741) {
                return -25;
        }
        else if (sample < 1953) {
                return -24;
        }
        else if (sample < 2191) {
                return -23;
        }
        else if (sample < 2459) {
                return -22;
        }
        else if (sample < 2759) {
                return -21;
        }
        else if (sample < 3095) {
                return -20;
        }
        else if (sample < 3473) {
                return -19;
        }
        else if (sample < 3896) {
                return -18;
        }
        else if (sample < 4906) {
                return -17;
        }
        else if (sample < 5504) {
                return -16;
        }
        else if (sample < 6175) {
                return -15;
        }
        else if (sample < 6929) {
                return -14;
        }
        else if (sample < 7775) {
                return -13;
        }
        else if (sample < 8723) {
                return -12;
        }
        else if (sample < 9788) {
                return -11;
        }
        else if (sample < 10982) {
                return -10;
        }
        else if (sample < 12322) {
                return -9;
        }
        else if (sample < 13825) {
                return -8;
        }
        else if (sample < 15512) {
                return -7;
        }
        else if (sample < 17406) {
                return -6;
        }
        else if (sample < 19529) {
                return -5;
        }
        else if (sample < 21912) {
                return -4;
        }
        else if (sample < 24586) {
                return -3;
        }
        else if (sample < 27586) {
                return -2;
        }
        else if (sample < 30951) {
                return -1;
        }
        else if (sample < 32768) {
                return 0;
        }
}

static inline guint32 colour (gdouble level) {
        /* receives the index of soundbar level and returns its color*/

        if (level < 0.55) {
                return l_green;
        }
        else if (level < 0.75) {
                return yellow;
        }
        else if (level < 0.90) {
                return orange;
        }
        else if (level < 1.0) {
                return red;
        }
        else
                return transparent_black;
}

static inline void horizontal_rendering (struct video_info * vi,
                                         gdouble peak,
                                         guint32 * vdata,
                                         guint16 l_b,
                                         guint16 r_b,
                                         gdouble levels,
                                         guint8 loudness) {
        for (gint h = l_b; h < r_b; h++) {
                if (peak > 0.0) {
                        gint num;
                        num = h * (vi->width) + ((vi->width) * peak);
                        vdata[num] = white;
                }

                for (gint i = 1; i <= loudness; i++) {
                        for (gint w = lvl_height * (levels - i) + 1; w < lvl_height * (levels - i + 1) - 1; w++) {
                                vdata[vi->width - w + (vi->width) * h] = colour (i / levels);
                        }
                }
        }
        return;
}

static inline void vertical_rendering (struct video_info * vi,
                                       gdouble peak,
                                       guint32 * vdata,
                                       guint16 l_b,
                                       guint16 r_b,
                                       gdouble levels,
                                       guint8 loudness) {
        for (gint h = l_b; h < r_b; h++) {
                if (peak > 0.0)
                {
                        guint16 peak_lvl;
                        gint num;
                        peak_lvl = lvl_height * (1 - peak) * levels;
                        num = h + (vi->width) * peak_lvl;
                        vdata[num] = white;
                }

                for (gint i = 1; i <= loudness; i++) {
                        for (gint w = lvl_height * (levels - i) + 1; w < lvl_height * (levels - i + 1) - 1; w++) {
                                vdata[h + (vi->width) * w] = colour (i / levels);
                        }
                }
        }
        return;
}

static inline gdouble * render (struct state * state,
                                struct video_info * vi,
                                struct audio_info * ai,
                                guint32 * vdata,
                                GstMapInfo amap,
                                gdouble * peaks,
                                guint8 channel_width1,
                                gdouble horizontal,
                                gint max_channel) {

        gdouble levels;
        gint16 *data_16 = (gint16 *)amap.data;
        guint16 channel_width;
        gint channels = ai->channels;
        gint size     = amap.size / sizeof (gint16);
        gint rate     = ai->rate;
        gdouble samples_per_ch = size / channels;
        /*   gint measurable = (rate / channels) / 200; */
        guint64 sum [MAX_CHANNEL_N] = { 0 };

        /* making transparent im */
        for (guint i = 0; i < vi->height * vi->width; i++) {
                vdata[i] = 0xff000000 || vdata[i];
        }

        /* calculations part */
        guint16 hor, vert;
        hor  = horizontal ? vi->height : vi->width;
        vert = horizontal ? vi->width : vi->height;

        if ((hor - 2 * (channels - 1)) / channels > (channel_width1 - 2) &&
            (channel_width1 - 2) > 0) {
                channel_width = (channel_width1 - 2);
        }
        else {
                channel_width = floor ((hor - 2 * (channels - 1)) / channels);
        }
        levels = floor(vert / lvl_height);


        for (gint ch = 0; ch < channels; ch++) {

                for (gint i = ch; i <= size; i += channels) {
                        sum[ch] += abs (data_16[i]);
                }

                gint new_vol = sum[ch] / samples_per_ch;
                /*   gdouble db = sample_to_db (new_vol); */
                gdouble db = 20.0 * (log10 ((gdouble)new_vol / 32768.0));
                gdouble vol = (40.0 + db) / 40.0;

                if (vol < 0.0) { vol = 0.0;}
                gdouble s = 0.05 * (gdouble)size / (gdouble)rate;

                /* rendering part */
                guint16 l_b = (hor - channel_width * (channels) - 2 * (channels)) / 2 +
                        channel_width * ch + ch * 2;
                guint16 r_b = (hor - channel_width * (channels) - 2 * (channels)) / 2 +
                        channel_width * (ch + 1) + ch * 2;
                guint8 loudness = rint(vol * levels);

                if (ch < max_channel)
                {
                        if (peaks[ch] <= vol) {
                                peaks[ch] = vol;
                        }
                        else {
                                peaks[ch] = peaks[ch] - s;
                        }

                        if (horizontal) {
                                horizontal_rendering (vi, peaks[ch], vdata, l_b, r_b, levels, loudness);
                        }
                        else {
                                vertical_rendering (vi, peaks[ch], vdata, l_b, r_b, levels, loudness);
                        }
                }
                else
                {
                        if (horizontal) {
                                horizontal_rendering (vi, 0.0, vdata, l_b, r_b, levels, loudness);
                        }
                        else {
                                vertical_rendering (vi, 0.0, vdata, l_b, r_b, levels, loudness);
                        }
                }
        }
        return 0;
}
