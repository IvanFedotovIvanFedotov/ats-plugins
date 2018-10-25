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
        if (sample < 6942) {
                return -20;
        }
        else if (sample < 7789) {
                return -19;
        }
        else if (sample < 8739) {
                return -18;
        }
        else if (sample < 9806) {
                return -17;
        }
        else if (sample < 11002) {
                return -16;
        }
        else if (sample < 12345) {
                return -15;
        }
        else if (sample < 13851) {
                return -14;
        }
        else if (sample < 15541) {
                return -13;
        }
        else if (sample < 17437) {
                return -12;
        }
        else if (sample < 19565) {
                return -11;
        }
        else if (sample < 21952) {
                return -10;
        }
        else if (sample < 24631) {
                return -9;
        }
        else if (sample < 27636) {
                return -8;
        }
        else if (sample < 31008) {
                return -7;
        }
        else if (sample < 34792) {
                return -6;
        }
        else if (sample < 39037) {
                return -5;
        }
        else if (sample < 43801) {
                return -4;
        }
        else if (sample < 49145) {
                return -3;
        }
        else if (sample < 55142) {
                return -2;
        }
        else if (sample < 61870) {
                return -1;
        }
        else if (sample < 65536) {
                return 0;
        }
}

static inline guint32 colour (gdouble level) {
        /* receives the index of soundbar level and returns its color*/
        if (level < 0.2) {
                return green;
        }
        else if (level < 0.4) {
                return l_green;
        }
        else if (level < 0.6) {
                return yellow;
        }
        else if (level < 0.8) {
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
        gint fps      = vi->fps;
        gint channels = ai->channels;
        gint size     = amap.size / sizeof (gint16);
        gint rate     = ai->rate;
        /*   gdouble samples_per_ch = ai->samples; */
        /*   gint measurable = (rate / channels) / 200; */
        /*    guint64 sum [MAX_CHANNEL_N] = { 0 }; */

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

                gint max = 0;
/*
                for (gint samp = ch;
                     samp <= size - measurable * channels;
                     samp += channels) {
                        gint64 sum = 0;
                        gint16 num = 0;

                        for (gint i = samp;
                             i <= samp + measurable * channels;
                             i += channels) {
                                if (i >= 0 && i <= size) {
                                        sum += (data_16[i] + 32768);
                                        num ++;
                                }
                        }
                        gint average_1 = sum / num;
                        if (average_1 > average ) {
                                average = average_1;
                        }

                        }*/
                for (gint i = ch; i < size; i += channels) {
                        if (data_16[i] + 32768 > max) {
                                max = (data_16[i] + 32768);
                        }
                }

                gint db = sample_to_db (max);
                gdouble vol = (gdouble)(20 + db) / 20;


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
