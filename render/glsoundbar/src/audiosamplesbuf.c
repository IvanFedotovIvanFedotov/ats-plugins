/*
 * GStreamer glsoundbar
 * Copyright (C) 2018 NIITV.
 * Ivan Fedotov<ivanfedotovmail@yandex.ru>
 *
 */

/**
 * SECTION:element-glsoundbar
 *
 * FIXME:Describe glsoundbar here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 *
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include "audiosamplesbuf.h"

#include <math.h>




void audiosamplesbuf_init(AudioSamplesBuf * filter){

  int i;

  filter->channels=0;
  filter->rate=0;
  filter->frames_num=0;
  filter->samples=NULL;
  filter->timedelta_average=0.0;

  filter->video_fps=25;

  filter->frames_max=0;
  filter->converter=NULL;
  filter->flags=GST_AUDIO_CONVERTER_FLAG_NONE;
  filter->out_format=GST_AUDIO_FORMAT_UNKNOWN;
  filter->options=NULL;

  filter->result.channels=0;

  for(i=0;i<64;i++){
    filter->result.loud_average[i]=0;
    filter->result.loud_peak[i]=0;
    filter->loud_average[i]=0;
    filter->loud_peak[i]=0;
  }

}



gboolean audiosamplesbuf_create(AudioSamplesBuf * filter,
                                    GstAudioInfo *ainfo, GstVideoInfo *vinfo,
                                    float audio_loud_speed, float audio_peak_speed){

  int i;

  audiosamplesbuf_free(filter);

  filter->audio_loud_speed=audio_loud_speed;
  filter->audio_peak_speed=audio_peak_speed;

  filter->channels=ainfo->channels;
  filter->rate=ainfo->rate;

  if(vinfo->fps_d==0)return FALSE;

  filter->video_fps=((float)vinfo->fps_n)/((float)vinfo->fps_d);

  if(filter->video_fps<=0.0)return FALSE;

  filter->out_info=gst_audio_info_new();
  gst_audio_info_init(filter->out_info);

  //выбор выходного формата, который будет поступать в функцию рисования S16LE
  filter->out_format=gst_audio_format_build_integer(TRUE,G_LITTLE_ENDIAN,16,16);
  filter->flags=GST_AUDIO_CONVERTER_FLAG_NONE;

  filter->result.channels=filter->channels;
  filter->result.rate=filter->rate;
  for(i=0;i<64;i++){
    filter->result.loud_average[i]=0;
    filter->result.loud_peak[i]=0;
    filter->loud_peak[i]=0.0;
    filter->loud_average[i]=0;
  }

  //без ресэмплинга частоты (выходная частота равна входной)
  gst_audio_info_set_format(filter->out_info, filter->out_format, ainfo->rate, ainfo->channels, ainfo->position);
  filter->converter=gst_audio_converter_new(filter->flags, ainfo, filter->out_info, NULL);

  if(!filter->converter)return FALSE;
  return TRUE;

}

//in_buf_all_samples_num - количество сэмплов всех каналов буфера in_buf
gboolean audiosamplesbuf_set_data(AudioSamplesBuf * filter, GstAudioInfo *ainfo,
                                            gpointer *in_buf, gint in_buf_all_samples_num){

  if(in_buf_all_samples_num / ainfo->channels > filter->frames_max){
    if(!filter->samples)g_free(filter->samples);
    filter->samples = g_new0 (gint16, in_buf_all_samples_num);
    filter->frames_max = in_buf_all_samples_num / ainfo->channels;
  }

  filter->frames_num = in_buf_all_samples_num / ainfo->channels;

  gboolean res;

  //работает только с layout = interleave
  gpointer in[1];
  gpointer out[1];

  in[0]=in_buf;
  out[0]=filter->samples;

  gint frames_num_in;
  gint frames_num_out;

  frames_num_in=in_buf_all_samples_num / ainfo->channels;
  frames_num_out=gst_audio_converter_get_out_frames(filter->converter, frames_num_in);

  if(frames_num_out!=filter->frames_num)return FALSE;

  res=gst_audio_converter_samples(filter->converter,
                              filter->flags,
                              in,
                              frames_num_in,
                              out,
                              frames_num_out);

  return res;

}

gint64 abs64(gint64 v){

  if(v<0)return -v;
  return v;

}

float abs_float(float v){

  if(v<0)return -v;
  return v;

}

gboolean audiosamplesbuf_proceed(AudioSamplesBuf * filter){

  gint i,fr;

  gint16 v16;

  gint64 loud_average_i64[64];
  float loud_average_val;

  float delitel_16=65536.0;

  if(!filter->samples || filter->frames_num<=0)return TRUE;
  if(filter->video_fps<=0.0)return FALSE;

  for(i=0;i<64;i++){
    loud_average_i64[i]=0;
  }

  for(fr=0;fr<filter->frames_num;fr++){
    for(i=0;i<filter->channels;i++){
      v16=filter->samples[fr*filter->channels+i];
      loud_average_i64[i]+=abs64((gint64)(v16));
    }
  }

  filter->timedelta_average=1.0/filter->video_fps;
  if(filter->timedelta_average<=0.0)return FALSE;

  float speed=8.0*filter->audio_loud_speed;

  for(i=0;i<filter->channels;i++){
    loud_average_i64[i]/=(gint64)filter->frames_num;
    loud_average_val=((float)loud_average_i64[i])/((float)delitel_16);
    filter->loud_average[i]=(((1.0/filter->timedelta_average)/speed)*filter->loud_average[i]+
                             loud_average_val)/((1.0/filter->timedelta_average)/speed+1.0);

    filter->loud_peak[i]-=filter->loud_peak[i]*0.03* filter->audio_peak_speed;
    if(filter->loud_peak[i]<filter->loud_average[i])filter->loud_peak[i]=filter->loud_average[i];

    if(filter->loud_average[i]<0.0)filter->loud_average[i]=0.0;
    if(filter->loud_average[i]>1.0)filter->loud_average[i]=1.0;
    if(filter->loud_peak[i]<0.0)filter->loud_peak[i]=0.0;
    if(filter->loud_peak[i]>1.0)filter->loud_peak[i]=1.0;
  }

  float v;

  for(i=0;i<filter->channels;i++){

    if(filter->loud_average[i]>0.0000000001f)v=20.0*logf(filter->loud_average[i])/logf(10.0);
    else v=-100.0;
    if(v<-100.0)v=-100.0;
    filter->result.loud_average[i]=v/100.0+1.0;

    if(filter->loud_peak[i]>0.0000000001f)v=20.0*logf(filter->loud_peak[i])/logf(10.0);
    else v=-100.0;
    if(v<-100.0)v=-100.0;
    filter->result.loud_peak[i]=v/100.0+1.0;

  }

  return TRUE;

}


void audiosamplesbuf_free(AudioSamplesBuf * filter){

  if(filter->samples!=NULL)g_free(filter->samples);
  filter->samples=NULL;

  if(filter->out_info!=NULL)gst_audio_info_free(filter->out_info);
  filter->out_info=NULL;

  if(filter->converter!=NULL)gst_audio_converter_free(filter->converter);
  filter->converter=NULL;

  if(filter->options!=NULL)gst_structure_free(filter->options);
  filter->options=NULL;

  audiosamplesbuf_init(filter);

}
