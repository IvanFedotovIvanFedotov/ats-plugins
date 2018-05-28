#include "gstsoundbar.h"
#include <math.h>

static const guint32 red     = 65535;
static const guint32 orange  = 65535 << 7;
static const guint32 yellow  = 65535 << 8;
static const guint32 l_green = 32265 << 12;
static const guint32 green   = 32265 << 8;
static const guint32 white   = 0xffffffff;
static const guint32 black   = 0;

static const guint8 lvl_height = 5;  /* soundbar level height in px */

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
    return black;
}

static inline horizontal_rendering (struct video_info * vi,
                                    gdouble peak,
                                    guint32 * vdata,
                                    guint16 l_b,
                                    guint16 r_b,
                                    gdouble levels,
                                    guint8 loudness) {
  for (gint h = l_b; h < r_b; h++) {
    gint num;
    num = h * (vi->width) + ((vi->width) * peak);
    vdata[num] = white;

    for (gint i = 1; i <= loudness; i++) {
      for (gint w = lvl_height * (levels - i) + 1; w < lvl_height * (levels - i + 1) - 1; w++) {
        vdata[vi->width - w + (vi->width) * h] = colour (i / levels);
      }
    }
  }
  return 0;
}

static inline vertical_rendering (struct video_info * vi,
                                  gdouble peak,
                                  guint32 * vdata,
                                  guint16 l_b,
                                  guint16 r_b,
                                  gdouble levels,
                                  guint8 loudness) {
  for (gint h = l_b; h < r_b; h++) {
    guint16 peak_lvl;
    gint num;
    peak_lvl = lvl_height * (1 - peak) * levels;
    num = h + (vi->width) * peak_lvl;
    vdata[num] = white;

    for (gint i = 1; i <= loudness; i++) {
      for (gint w = lvl_height * (levels - i) + 1; w < lvl_height * (levels - i + 1) - 1; w++) {
        vdata[h + (vi->width) * w] = colour (i / levels);
      }
    }
  }
  return 0;
}

static inline gdouble * render (struct state * state,
                                struct video_info * vi,
                                struct audio_info * ai,
                                guint32 * vdata,
                                GstMapInfo amap,
                                gdouble * peaks,
                                guint8 channel_width1,
                                gdouble horizontal) {

  gdouble levels;
  guint16 *data_16 = (guint16 *)amap.data;
  guint16 channel_width;
  gint rate = ai->rate;
  gint fps  = vi->fps;
  guint8 measurable = rate / 200;   /*  1 s / 200 = 5 ms */

  /* calculations part */

  guint16 width, height;
  width = horizontal ? vi->height : vi->width;
  height = horizontal ? vi->width : vi->height;

  if ((width - 2 * (ai->channels - 1)) / ai->channels > (channel_width1 - 2) &&
      (channel_width1 - 2) > 0) {
    channel_width = (channel_width1 - 2);
  }
  else {
    channel_width = floor ((width - 2 * (ai->channels - 1)) / ai->channels);
  }
  levels = floor(height / lvl_height);

  for (gint ch = 0; ch < (ai->channels); ch++) {

      gdouble ppm  = 0.0;

      for (gint k = ch; k < amap.size; k = k + ai->channels) {
        gdouble sum = 0.0;
        guint16 num = 0;
        for (gint j = k - measurable; j < k + measurable; j = j + ai->channels) {
          if (j < 0 || j > amap.size - 1) {
            continue;
          }
          gdouble sample = ((guint16)data_16[j]) / 65536.0;;
          sum += sample;
          num ++;
        }
        if (num > 0) {
          if (sum / num > ppm) {
            ppm = (sum / num);
          }
        }
      }

      gdouble s = 0.1 / (gdouble)fps;

      guint8 loudness = rint(ppm * levels);
      if (peaks[ch] <= ppm) {
        peaks[ch] = ppm;
      }
      else {
        peaks[ch] = peaks[ch] - s;
      }


      /* rendering part */

      guint16 l_b = (width - channel_width * (ai->channels) - 2 * (ai->channels)) / 2 +
        channel_width * ch + ch * 2;
      guint16 r_b = (width - channel_width * (ai->channels) - 2 * (ai->channels)) / 2 +
        channel_width * (ch + 1) + ch * 2;

      if (horizontal) {
        horizontal_rendering (vi, peaks[ch], vdata, l_b, r_b, levels, loudness);
      }
      else {
        vertical_rendering (vi, peaks[ch], vdata, l_b, r_b, levels, loudness);
      }
  }
  return 0;
}
