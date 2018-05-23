#include "gstsoundbar.h"
#include <math.h>

const guint32 red     = 65535;
const guint32 orange  = 65535 << 7;
const guint32 yellow  = 65535 << 8;
const guint32 l_green = 32265 << 12;
const guint32 green   = 32265 << 8;
const guint32 white   = 0xffffffff;


const guint8 level_height = 5;  /* soundbar levels */

const guint16 channel_width1 = 10; /* soundbar width in px */

guint32 colour (gdouble level) {
  /* function receives the the index of soundbar level and returns its color*/

            if (level < 0.2)
              {
                return green;
              }
            else if (level < 0.4)
              {
                return l_green;
              }
            else if (level < 0.6)
              {
                return yellow;
              }
            else if (level < 0.8)
              {
                return orange;
              }
            else if (level < 1.0)
              {
                return red;
              }
            else
              return 0;
}

inline gdouble * render (struct state * state,
                    struct video_info * vi,
                    struct audio_info * ai,
                    guint32 * vdata,
                    GstMapInfo amap,
                    gdouble * peaks) {

  /*  guint32 fing = adata[0] + (((guint32) adata[1]) << 16);*/
  /* guint8 loudness_1 = rand() % 10 +1;*/
  gdouble rms, avg, sum1, sum, ppm, sum2;
  gdouble levels = (vi->height)/level_height;
  guint16 *data_16 = (guint16 *)amap.data;
  guint16 channel_width, num, num2;
  if (vi->width / ai->channels > channel_width1)
    {
      channel_width = channel_width1;
    }
  else
    {
      channel_width = floor ((vi->width - 2*(ai->channels-1))/ ai->channels);
    }
  for (gint ch = 0; ch < ai->channels; ch++)
    {
      rms  = 0.0;
      sum  = 0.0;
      sum1 = 0.0;
      num  = 0;
      ppm = 0.0;

      for (gint k = ch; k < amap.size; k = k + ai->channels) {
        gdouble sample = ((guint16)data_16[k]) / 65536.0;
        sum += sample * sample;
        sum1 += sample;
        num++;
        sum2 = 0.0;
        num2 = 0;
        for (gint j = k - (270 * ai->channels); j < k + (270 * ai->channels); j = j + ai->channels) {
          gdouble sample = ((guint16)data_16[j]) / 65536.0;
          sum2 += sample;
          num2 ++;
        }
        if (num2>0)
          {
            if (sum2 / num2 > ppm) {
              ppm = (sum2 / num2);
            }
          }
      }
      if (num > 0) {
        avg = sum1 / num;
        rms = sqrt (sum / num);
      }
      else {
        avg = 0.0;
        rms = 0.0;
      }
      if (rms > 1.0) {rms = 1.0;}

      guint8 loudness = rint(ppm * levels);
      if (peaks[ch]<ppm)
        {
          peaks[ch]=ppm;
        }
      else
        {
          peaks[ch]=peaks[ch]-0.0015;
        }
      guint16 l_b = (vi->width-channel_width*(ai->channels)-2*ai->channels)/2+channel_width*ch+ch*2;
      guint16 r_b = (vi->width-channel_width*(ai->channels)-2*ai->channels)/2+channel_width*(ch+1)+ch*2;
      for (guint16 w = l_b; w < r_b; w++) {
        guint16 peak_lvl = level_height*(1-peaks[ch])*levels;
        gint num = w+vi->width*peak_lvl;
        vdata[num] = white;
        for (guint8 i = 1; i <= loudness; i++) {
          for (guint16 h = level_height*(levels-i)+1; h < level_height*(levels-i+1)-1; h++)  {
            guint32 color = colour (i/levels);
            vdata[w+vi->width*h] = color;
          }
        }
      }
    }
  return peaks;
}
