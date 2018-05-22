#include "gstsoundbar.h"
#include <math.h>

const guint32 red    = 65535;
const guint32 orange = 65535 << 7;
const guint32 yellow = 65535 << 8;
const guint32 l_green  = 32265 << 12;
const guint32 green  = 32265 << 8;


const guint8 soundbar_levels = 10;  /* soundbar levels */
const guint8 levels          = 12;  /* soundbar levels + 2 (top and bottom) */

const gdouble soundbar_width = 0.1; /* soundbar width / video width */

guint32 colour (guint8 level) {
  /* function receives the the index of soundbar level and returns its color*/
  switch (level)
          {
          case 1: case 2:
            {
              return green;
              break;
            }
          case 3: case 4:
            {
              return l_green;
              break;
            }
          case 5: case 6:
            {
              return yellow;
              break;
            }
          case 7: case 8:
            {
              return orange;
              break;
            }
          case 9: case 10:
            {
              return red;
              break;
            }
          default:
            return 0;
          };
}

inline void render (struct state * state,
                    struct video_info * vi,
                    struct audio_info * ai,
                    guint32 * vdata,
                    GstMapInfo amap) {

  /*  guint32 fing = adata[0] + (((guint32) adata[1]) << 16);*/
  /* guint8 loudness_1 = rand() % 10 +1;*/
  gdouble sum, rms;
  sum = 0.0;
  guint16 *data_16 = (guint16 *)amap.data;
  for (gint i = 0; i < amap.size; i=i+2)
    {
      gdouble sample = ((guint16)data_16[i]) / 32768.0;
      sum += (sample * sample);
    }
  rms = sqrt(sum / amap.size / 2);
  g_print ("rms is %f\n", rms);
  gdouble dB = 20 * log10(rms);
  g_print ("level is %f dB\n", dB);
  guint8 loudness = rint(rms * 10);
  for (guint16 w = (vi->width)*(1-soundbar_width*2); w < (vi->width)*(1-soundbar_width); w++) {
    for (guint8 i = 1; i <= loudness; i++) {
      for (guint16 h = (vi->height)/levels*(levels-i)+2; h < (vi->height)/levels*(levels-i+1)-2; h++) {
        guint32 color = colour (i);
        vdata[w+(vi->width)*h] = color;
      }
    }
  }
}
