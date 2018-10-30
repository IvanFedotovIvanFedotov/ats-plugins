/*
 * Copyright (C) <2011> Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) <2015> Luis de Bethencourt <luis@debethencourt.com>
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2002,2007 David A. Schleef <ds@schleef.org>
 * Copyright (C) 2008 Julien Isorce <julien.isorce@gmail.com>
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
 * Copyright (C) 2018 NIITV. Ivan Fedotov<ivanfedotovmail@yandex.ru>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef __GST_GLSOUNDBAR_H__
#define __GST_GLSOUNDBAR_H__

#include <gst/gst.h>

#include <gst/video/video.h>
#include <gst/audio/audio.h>
#include <gst/base/gstadapter.h>

#include <gst/gl/gl.h>

#include "audiosamplesbuf.h"
#include "gldrawing.h"



G_BEGIN_DECLS




#define GST_TYPE_GLSOUNDBAR \
  (gst_glsoundbar_get_type())
#define GST_GLSOUNDBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_GLSOUNDBAR,GstGLSoundbar))
#define GST_GLSOUNDBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_GLSOUNDBAR,GstGLSoundbarClass))
#define GST_IS_GLSOUNDBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_GLSOUNDBAR))
#define GST_IS_GLSOUNDBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_GLSOUNDBAR))

typedef struct _GstGLSoundbar      GstGLSoundbar;
typedef struct _GstGLSoundbarClass GstGLSoundbarClass;
typedef struct _GstGLSoundbarPrivate GstGLSoundbarPrivate;


enum {

 GLSOUND_BAR_DRAW_DIRECTION_TO_UP=0,
 GLSOUND_BAR_DRAW_DIRECTION_TO_RIGHT

};

struct _GstGLSoundbar
{

  GstElement element;
  /* <private> */
  GstGLSoundbarPrivate *priv;
  guint req_spf;                /* min samples per frame wanted by the subclass */
  /* video state */
  GstVideoInfo vinfo;
  /* audio state */
  GstAudioInfo ainfo;

  /* filter specific data */
  GstGLFramebuffer *fbo;
  GstGLMemory *out_tex;
  GstGLDisplay *display;
  GstGLContext *context, *other_context;
  gboolean gl_result;

  // -- gl selection draw method
  gpointer src_impl;
  //GstClockTime running_time;
  time_t prev_time;
  time_t current_time;

  gint64 timestamp_offset;              /* base offset */
  GstClockTime running_time;            /* total running time */
  gint64 n_frames;

  AudioSamplesBuf audio_samples_buf;

  GlDrawing gl_drawing;
  gboolean gl_drawing_created;

  GTimeVal gl_draw_timeprev;
  GTimeVal gl_draw_timecurr;

  gint bars_draw_direction;
  gfloat bar_aspect;
  gfloat bar_risc_len;
  gfloat bar_risc_step;
  gfloat peak_height;
  gfloat audio_loud_speed;
  gfloat audio_peak_speed;
  gboolean bar_aspect_auto;
  guint bg_color;

};

struct _GstGLSoundbarClass
{
  GstElementClass parent_class;

  //public
  GstGLAPI supported_gl_api;

};



//GST_EXPORT
GType gst_glsoundbar_get_type (void);

G_END_DECLS





#endif /* __GST_GLSOUNDBAR_H__ */
