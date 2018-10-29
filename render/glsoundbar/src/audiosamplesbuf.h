/*
 * GStreamer glsoundbar
 * Copyright (C) 2018 NIITV.
 * Ivan Fedotov<ivanfedotovmail@yandex.ru>
 *
 */

#ifndef __GST_AUDIOSAMPLESBUF_H__
#define __GST_AUDIOSAMPLESBUF_H__

#include <gst/gst.h>

#include <gst/audio/audio.h>
#include <gst/video/video.h>




typedef struct _AudioSamplesBuf      AudioSamplesBuf;

typedef struct _ResultData ResultData;


struct _ResultData{

  gint channels;
  gint rate;

  float loud_average[64];
  float loud_peak[64];

};


struct _AudioSamplesBuf
{

  GObject element;

  gint channels;
  gint rate;
  //Актуальное количество фреймов в буфере
  gint frames_num;
  gint16 *samples;

  ResultData result;

  float video_fps;

  float loud_peak[64];
  float loud_average[64];
  float timedelta_average;

  float audio_loud_speed;
  float audio_peak_speed;

  //Максимальное количество сэмплов в буфере (на 1 канал)
  //размер массива samples[frames_max*channels]
  //
  //размер буфера увеличивается, если размер входного массива превышает frames_max
  gint frames_max;

  GstAudioConverter *converter;
  GstAudioConverterFlags flags;
  GstStructure *options;

  //То к чему должны преобразовываться входные сэмплы
  GstAudioInfo *out_info;
  GstAudioFormat out_format;


};


void audiosamplesbuf_init(AudioSamplesBuf * filter);
gboolean audiosamplesbuf_create(AudioSamplesBuf * filter,
                                    GstAudioInfo *ainfo, GstVideoInfo *vinfo,
                                    float audio_loud_speed, float audio_peak_speed);
gboolean audiosamplesbuf_set_data(AudioSamplesBuf * filter, GstAudioInfo *ainfo, gpointer *in_buf,
                                  gint in_buf_all_samples_num);
gboolean audiosamplesbuf_proceed(AudioSamplesBuf * filter);
void audiosamplesbuf_free(AudioSamplesBuf * filter);



#endif /* __GST_AUDIOSAMPLESBUF_H__ */
