/*
 * GStreamer gstaudiovisualizer
 * Copyright (C) <2011> Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) <2015> Luis de Bethencourt <luis@debethencourt.com>
 *
 * gstaudiovisualizer.c: base class for audio visualisation elements
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
 *
 * GStreamer gstgltestsrc
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2002,2007 David A. Schleef <ds@schleef.org>
 * Copyright (C) 2008 Julien Isorce <julien.isorce@gmail.com>
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
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
 *
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

#include <gst/gl/gl.h>
#include <gst/gl/gstglutils.h>

#include <time.h>

#include <string.h>

#include <gst/audio/audio.h>
#include <gst/video/video.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

#include "gstglsoundbar.h"
#include "audiosamplesbuf.h"
#include "gldrawing.h"


GST_DEBUG_CATEGORY_STATIC (gst_glsoundbar_debug);
#define GST_CAT_DEFAULT gst_glsoundbar_debug


#define USE_PEER_BUFFERALLOC
#define SUPPORTED_GL_APIS (GST_GL_API_OPENGL | GST_GL_API_OPENGL3 | GST_GL_API_GLES2)

// Filter signals and args
enum
{
  // FILL ME
  LAST_SIGNAL,
  PROP_BARS_DIRECTION,//wave
  PROP_BAR_ASPECT,
  PROP_BAR_RISC_LEN,
  PROP_BAR_RISC_STEP,
  PROP_PEAK_HEIGHT_PIXELS,
  PROP_AUDIO_LOUD_SPEED,
  PROP_AUDIO_PEAK_SPEED,
  PROP_BAR_ASPECT_AUTO,

  PROP_BG_COLOR_R,
  PROP_BG_COLOR_G,
  PROP_BG_COLOR_B,
  PROP_BG_COLOR_A,

  PROP_TIMESTAMP_OFFSET

};

enum
{
  PROP_0,
};



#define gst_glsoundbar_parent_class parent_class
G_DEFINE_TYPE (GstGLSoundbar, gst_glsoundbar, GST_TYPE_ELEMENT);


#define GST_GLSOUNDBAR_GET_PRIVATE(obj)  \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_GLSOUNDBAR, GstGLSoundbarPrivate))


struct _GstGLSoundbarPrivate
{
  gboolean negotiated;

  GstBufferPool *pool;
  gboolean pool_active;
  GstAllocator *allocator;
  GstAllocationParams params;
  GstQuery *query;

  // pads
  GstPad *srcpad, *sinkpad;

  GstAdapter *adapter;

  GstBuffer *inbuf;
  GstBuffer *tempbuf;
  GstVideoFrame tempframe;

  guint spf;                    // samples per video frame
  guint64 frame_duration;

  // QoS stuff  |  with LOCK
  gdouble proportion;
  GstClockTime earliest_time;

  guint dropped;                // frames dropped / not dropped
  guint processed;

  // configuration mutex
  GMutex config_lock;

  GstSegment segment;

};

static GstStaticPadTemplate gst_glsoundbar_src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "
        "format = (string) RGBA, "
        "width = " GST_VIDEO_SIZE_RANGE ", "
        "height = " GST_VIDEO_SIZE_RANGE ", "
        "framerate = " GST_VIDEO_FPS_RANGE ","
        "texture-target = (string) 2D")
    );


static GstStaticPadTemplate gst_glsoundbar_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "layout = (string) interleaved"
        )
    );

static void gst_glsoundbar_finalize (GObject * object);
static void gst_glsoundbar_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_glsoundbar_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_glsoundbar_dispose (GObject * object);
static gboolean gst_glsoundbar_src_negotiate (GstGLSoundbar * scope);
static gboolean gst_glsoundbar_src_setcaps (GstGLSoundbar *
    scope, GstCaps * caps);
static gboolean gst_glsoundbar_sink_setcaps (GstGLSoundbar *
    scope, GstCaps * caps);
static GstFlowReturn gst_glsoundbar_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buffer);
static gboolean gst_glsoundbar_src_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_glsoundbar_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static gboolean gst_glsoundbar_src_query (GstPad * pad,
    GstObject * parent, GstQuery * query);
static GstStateChangeReturn gst_glsoundbar_change_state (GstElement *
    element, GstStateChange transition);
static gboolean gst_glsoundbar_do_bufferpool (GstGLSoundbar * scope,
    GstCaps * outcaps);
static gboolean
default_decide_allocation (GstGLSoundbar * scope, GstQuery * query);
static gint gst_glsoundbar_get_draw_direction(GstGLSoundbar *scope);




gint gst_glsoundbar_get_draw_direction(GstGLSoundbar *scope){

  return scope->bars_draw_direction;

}

static void
gst_glsoundbar_reset (GstGLSoundbar * scope)
{
  gst_adapter_clear (scope->priv->adapter);
  gst_segment_init (&scope->priv->segment, GST_FORMAT_UNDEFINED);

  GST_OBJECT_LOCK (scope);
  scope->priv->proportion = 1.0;
  scope->priv->earliest_time = -1;
  scope->priv->dropped = 0;
  scope->priv->processed = 0;
  GST_OBJECT_UNLOCK (scope);
}

static gboolean
gst_glsoundbar_sink_setcaps (GstGLSoundbar * scope, GstCaps * caps)
{
  GstAudioInfo info;

  if (!gst_audio_info_from_caps (&info, caps))
    goto wrong_caps;

  scope->ainfo = info;

  GST_DEBUG_OBJECT (scope, "audio: channels %d, rate %d",
      GST_AUDIO_INFO_CHANNELS (&info), GST_AUDIO_INFO_RATE (&info));

  if (!gst_glsoundbar_src_negotiate (scope)) {
    goto not_negotiated;
  }

  return TRUE;

  // Errors
wrong_caps:
  {
    GST_WARNING_OBJECT (scope, "could not parse caps");
    return FALSE;
  }
not_negotiated:
  {
    GST_WARNING_OBJECT (scope, "failed to negotiate");
    return FALSE;
  }

}

static gboolean
gst_glsoundbar_src_setcaps (GstGLSoundbar * scope, GstCaps * caps)
{
  GstVideoInfo info;

  gboolean res;

  if (!gst_video_info_from_caps (&info, caps))
    goto wrong_caps;

  scope->vinfo = info;

  scope->priv->frame_duration = gst_util_uint64_scale_int (GST_SECOND,
      GST_VIDEO_INFO_FPS_D (&info), GST_VIDEO_INFO_FPS_N (&info));
  scope->priv->spf =
      gst_util_uint64_scale_int (GST_AUDIO_INFO_RATE (&scope->ainfo),
      GST_VIDEO_INFO_FPS_D (&info), GST_VIDEO_INFO_FPS_N (&info));
  scope->req_spf = scope->priv->spf;

  if (scope->priv->tempbuf) {
    gst_video_frame_unmap (&scope->priv->tempframe);
    gst_buffer_unref (scope->priv->tempbuf);
  }
  scope->priv->tempbuf = gst_buffer_new_wrapped (g_malloc0 (scope->vinfo.size),
      scope->vinfo.size);
  gst_video_frame_map (&scope->priv->tempframe, &scope->vinfo,
      scope->priv->tempbuf, GST_MAP_READWRITE);

  g_free (scope->flt);
  scope->flt = g_new0 (gdouble, 6 * GST_AUDIO_INFO_CHANNELS (&scope->ainfo));

  GST_DEBUG_OBJECT (scope, "video: dimension %dx%d, framerate %d/%d",
      GST_VIDEO_INFO_WIDTH (&info), GST_VIDEO_INFO_HEIGHT (&info),
      GST_VIDEO_INFO_FPS_N (&info), GST_VIDEO_INFO_FPS_D (&info));
  GST_DEBUG_OBJECT (scope, "blocks: spf %u, req_spf %u",
      scope->priv->spf, scope->req_spf);

  gst_pad_set_caps (scope->priv->srcpad, caps);

  if(!audiosamplesbuf_create(&scope->audio_samples_buf, &scope->ainfo, &scope->vinfo,
                                 scope->audio_loud_speed, scope->audio_peak_speed))
    goto wrong_audiosamplesbuf_create;
  // find a pool for the negotiated caps now
  res = gst_glsoundbar_do_bufferpool (scope, caps);
  gst_caps_unref (caps);

  return res;

  // ERRORS
wrong_caps:
  {
    gst_caps_unref (caps);
    GST_DEBUG_OBJECT (scope, "error parsing caps");
    return FALSE;
  }

wrong_audiosamplesbuf_create:
  {
    GST_WARNING_OBJECT (scope, "failed audiosamplesbuf_create");
    return FALSE;
  }
}

static gboolean
gst_glsoundbar_src_negotiate (GstGLSoundbar * scope)
{
  GstCaps *othercaps, *target;
  GstStructure *structure;
  GstCaps *templ;
  gboolean ret;

  templ = gst_pad_get_pad_template_caps (scope->priv->srcpad);

  GST_DEBUG_OBJECT (scope, "performing negotiation");

  // see what the peer can do
  othercaps = gst_pad_peer_query_caps (scope->priv->srcpad, NULL);
  if (othercaps) {
    target = gst_caps_intersect (othercaps, templ);
    gst_caps_unref (othercaps);
    gst_caps_unref (templ);

    if (gst_caps_is_empty (target))
      goto no_format;

    target = gst_caps_truncate (target);
  } else {
    target = templ;
  }

  target = gst_caps_make_writable (target);
  structure = gst_caps_get_structure (target, 0);
  gst_structure_fixate_field_nearest_int (structure, "width", 640);
  gst_structure_fixate_field_nearest_int (structure, "height", 480);
  gst_structure_fixate_field_nearest_fraction (structure, "framerate", 25, 1);
  if (gst_structure_has_field (structure, "pixel-aspect-ratio"))
    gst_structure_fixate_field_nearest_fraction (structure,
        "pixel-aspect-ratio", 1, 1);

  target = gst_caps_fixate (target);

  GST_DEBUG_OBJECT (scope, "final caps are %" GST_PTR_FORMAT, target);

  ret = gst_glsoundbar_src_setcaps (scope, target);

  return ret;

no_format:
  {
    gst_caps_unref (target);
    return FALSE;
  }
}


// takes ownership of the pool, allocator and query
static gboolean
gst_glsoundbar_set_allocation (GstGLSoundbar * scope,
    GstBufferPool * pool, GstAllocator * allocator,
    GstAllocationParams * params, GstQuery * query)
{
  GstAllocator *oldalloc;
  GstBufferPool *oldpool;
  GstQuery *oldquery;
  GstGLSoundbarPrivate *priv = scope->priv;

  //breakpoint_01//
  GST_OBJECT_LOCK (scope);
  oldpool = priv->pool;
  priv->pool = pool;
  priv->pool_active = FALSE;

  oldalloc = priv->allocator;
  priv->allocator = allocator;

  oldquery = priv->query;
  priv->query = query;

  if (params)
    priv->params = *params;
  else
    gst_allocation_params_init (&priv->params);
  GST_OBJECT_UNLOCK (scope);

  if (oldpool) {
    GST_DEBUG_OBJECT (scope, "deactivating old pool %p", oldpool);
    gst_buffer_pool_set_active (oldpool, FALSE);
    gst_object_unref (oldpool);
  }
  if (oldalloc) {
    gst_object_unref (oldalloc);
  }
  if (oldquery) {
    gst_query_unref (oldquery);
  }
  return TRUE;
}


static gboolean
gst_glsoundbar_do_bufferpool (GstGLSoundbar * scope,
    GstCaps * outcaps)
{
  GstQuery *query;
  gboolean result = TRUE;
  GstBufferPool *pool = NULL;
  GstAllocator *allocator;
  GstAllocationParams params;

  // not passthrough, we need to allocate
  // find a pool for the negotiated caps now
  GST_DEBUG_OBJECT (scope, "doing allocation query");
  query = gst_query_new_allocation (outcaps, TRUE);

  if (!gst_pad_peer_query (scope->priv->srcpad, query)) {
    // not a problem, we use the query defaults
    GST_DEBUG_OBJECT (scope, "allocation query failed");
  }

  GST_DEBUG_OBJECT (scope, "calling decide_allocation");

  result = default_decide_allocation (scope, query);

  GST_DEBUG_OBJECT (scope, "ALLOCATION (%d) params: %" GST_PTR_FORMAT, result,
      query);

  if (!result)
    goto no_decide_allocation;

  // we got configuration from our peer or the decide_allocation method, parse them
  if (gst_query_get_n_allocation_params (query) > 0) {
    gst_query_parse_nth_allocation_param (query, 0, &allocator, &params);
  } else {
    allocator = NULL;
    gst_allocation_params_init (&params);
  }

  if (gst_query_get_n_allocation_pools (query) > 0)
    gst_query_parse_nth_allocation_pool (query, 0, &pool, NULL, NULL, NULL);

  // now store
  result =
      gst_glsoundbar_set_allocation (scope, pool, allocator, &params,
      query);

  return result;

  //return TRUE;
  // Errors
no_decide_allocation:
  {
    GST_WARNING_OBJECT (scope, "Subclass failed to decide allocation");
    gst_query_unref (query);

    return result;
  }
}

static gboolean
gst_gl_test_src_init_shader (GstGLSoundbar * src)
{

  if(!gst_gl_context_get_gl_api(src->context)){
    return FALSE;
  }
  return TRUE;
}

static gboolean
_find_local_gl_context (GstGLSoundbar * src)
{

  if (gst_gl_query_local_gl_context (GST_ELEMENT (src), GST_PAD_SRC,
          &src->context))
    return TRUE;
  return FALSE;
}


static void
_src_generate_fbo_gl (GstGLContext * context, GstGLSoundbar * src)
{
  src->fbo = gst_gl_framebuffer_new_with_default_depth (src->context,
      GST_VIDEO_INFO_WIDTH (&src->vinfo),
      GST_VIDEO_INFO_HEIGHT (&src->vinfo));
}


static gboolean
default_decide_allocation (GstGLSoundbar * scope, GstQuery * query)
{

  GstGLSoundbar *src = scope;
  GstBufferPool *pool = NULL;
  GstStructure *config;
  GstCaps *caps;
  guint min, max, size;
  gboolean update_pool;
  GError *error = NULL;


  if (!gst_gl_ensure_element_data (src, &src->display, &src->other_context)){
    return FALSE;
  }

  src->timestamp_offset = 0;
  src->running_time = 0;
  src->n_frames = 0;

  gst_gl_display_filter_gl_api (src->display, SUPPORTED_GL_APIS);

  _find_local_gl_context (src);

  if (!src->context) {
    GST_OBJECT_LOCK (src->display);
    do {
      if (src->context) {
        gst_object_unref (src->context);
        src->context = NULL;
      }
      // just get a GL context.  we don't care
      src->context =
          gst_gl_display_get_gl_context_for_thread (src->display, NULL);
      if (!src->context) {
        if (!gst_gl_display_create_context (src->display, src->other_context,
                &src->context, &error)) {
          GST_OBJECT_UNLOCK (src->display);
          goto context_error;
        }
      }
    } while (!gst_gl_display_add_context (src->display, src->context));
    GST_OBJECT_UNLOCK (src->display);
  }

  if ((gst_gl_context_get_gl_api (src->context) & SUPPORTED_GL_APIS) == 0){
    goto unsupported_gl_api;
  }

  gst_gl_context_thread_add (src->context,
      (GstGLContextThreadFunc) _src_generate_fbo_gl, src);
  if (!src->fbo){
    goto context_error;
  }

  gst_query_parse_allocation (query, &caps, NULL);

  if (gst_query_get_n_allocation_pools (query) > 0) {
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);

    update_pool = TRUE;
  } else {
    GstVideoInfo vinfo;

    gst_video_info_init (&vinfo);
    gst_video_info_from_caps (&vinfo, caps);
    size = vinfo.size;
    min = max = 0;
    update_pool = FALSE;
  }

  if (!pool || !GST_IS_GL_BUFFER_POOL (pool)) {
    // can't use this pool
    if (pool)
      gst_object_unref (pool);
    pool = gst_gl_buffer_pool_new (src->context);
  }
  config = gst_buffer_pool_get_config (pool);

  gst_buffer_pool_config_set_params (config, caps, size, min, max);
  gst_buffer_pool_config_add_option (config, GST_BUFFER_POOL_OPTION_VIDEO_META);
  if (gst_query_find_allocation_meta (query, GST_GL_SYNC_META_API_TYPE, NULL))
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_GL_SYNC_META);
  gst_buffer_pool_config_add_option (config,
      GST_BUFFER_POOL_OPTION_VIDEO_GL_TEXTURE_UPLOAD_META);

  gst_buffer_pool_set_config (pool, config);

  if (update_pool)
    gst_query_set_nth_allocation_pool (query, 0, pool, size, min, max);
  else
    gst_query_add_allocation_pool (query, pool, size, min, max);

  gst_gl_test_src_init_shader (src);

  gst_object_unref (pool);

  return TRUE;

unsupported_gl_api:
  {
    GstGLAPI gl_api = gst_gl_context_get_gl_api (src->context);
    gchar *gl_api_str = gst_gl_api_to_string (gl_api);
    gchar *supported_gl_api_str = gst_gl_api_to_string (SUPPORTED_GL_APIS);
    GST_ELEMENT_ERROR (src, RESOURCE, BUSY,
        ("GL API's not compatible context: %s supported: %s", gl_api_str,
            supported_gl_api_str), (NULL));

    g_free (supported_gl_api_str);
    g_free (gl_api_str);

    return FALSE;
  }
context_error:
  {
    if (error) {
      GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, ("%s", error->message),
          (NULL));
      g_clear_error (&error);
    } else {
      GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (NULL), (NULL));
    }
    if (src->context)
      gst_object_unref (src->context);
    src->context = NULL;

    return FALSE;
  }


}

static GstFlowReturn
default_prepare_output_buffer (GstGLSoundbar * scope, GstBuffer ** outbuf)
{
  GstGLSoundbarPrivate *priv;

  priv = scope->priv;

  g_assert (priv->pool != NULL);

  // we can't reuse the input buffer
  if (!priv->pool_active) {
    GST_DEBUG_OBJECT (scope, "setting pool %p active", priv->pool);
    if (!gst_buffer_pool_set_active (priv->pool, TRUE))
      goto activate_failed;
    priv->pool_active = TRUE;
  }
  GST_DEBUG_OBJECT (scope, "using pool alloc");

  return gst_buffer_pool_acquire_buffer (priv->pool, outbuf, NULL);

  // ERRORS
activate_failed:
  {
    GST_ELEMENT_ERROR (scope, RESOURCE, SETTINGS,
        ("failed to activate bufferpool"), ("failed to activate bufferpool"));
    return GST_FLOW_ERROR;
  }
}



static void
gst_glsoundbar_finalize (GObject * object)
{
  GstGLSoundbar *scope = GST_GLSOUNDBAR (object);

  g_mutex_clear(&scope->priv->config_lock);

  if(scope->gl_drawing_created==TRUE){
    gldraw_close (scope->context, &scope->gl_drawing, &scope->audio_samples_buf.result);
    scope->gl_drawing_created=FALSE;
  }

  if (scope->flt) {
    g_free (scope->flt);
    scope->flt = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}



static void
gst_gl_test_src_gl_stop (GstGLContext * context, GstGLSoundbar * src)
{

  if (src->fbo)
    gst_object_unref (src->fbo);
  src->fbo = NULL;

  if(src->gl_drawing_created==TRUE){
    gldraw_close (src->context, &src->gl_drawing, &src->audio_samples_buf.result);
    src->gl_drawing_created=FALSE;
  }

  src->src_impl = NULL;

}


static void
gst_glsoundbar_dispose (GObject * object)
{
  GstGLSoundbar *scope = GST_GLSOUNDBAR (object);

  if (scope->context){
    gst_gl_context_thread_add (scope->context, (GstGLContextThreadFunc) gst_gl_test_src_gl_stop, scope);
    GTimeVal timev;
    g_get_current_time(&timev);
    g_time_val_add(&timev,10);
  }
  if (scope->context)
    gst_object_unref (scope->context);
  scope->context = NULL;

  audiosamplesbuf_free(&scope->audio_samples_buf);

  if (scope->priv->adapter) {
    g_object_unref (scope->priv->adapter);
    scope->priv->adapter = NULL;
  }
  if (scope->priv->inbuf) {
    gst_buffer_unref (scope->priv->inbuf);
    scope->priv->inbuf = NULL;
  }
  if (scope->priv->tempbuf) {
    gst_video_frame_unmap (&scope->priv->tempframe);
    gst_buffer_unref (scope->priv->tempbuf);
    scope->priv->tempbuf = NULL;
  }
  if (scope->priv->config_lock.p) {
    g_mutex_clear (&scope->priv->config_lock);
    scope->priv->config_lock.p = NULL;
  }
  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static gboolean
gst_gl_test_src_callback (gpointer stuff)
{
  GstGLSoundbar *src = GST_GLSOUNDBAR (stuff);

  gboolean res;

  if(src->gl_drawing_created==FALSE){

    src->gl_result =  gldraw_init (src->context, &src->gl_drawing, &src->audio_samples_buf.result,
                                                src->vinfo.width, src->vinfo.height,
                                                gst_glsoundbar_get_draw_direction(src),
                                                src->bar_aspect, src->bar_risc_len, src->bar_risc_step,
                                                src->peak_height,
                                                src->bar_aspect_auto,
                                                src->bg_color_r,
                                                src->bg_color_g,
                                                src->bg_color_b,
                                                src->bg_color_a);

    src->gl_drawing_created=TRUE;

    if (!src->gl_result) {
      GST_ERROR_OBJECT (src, "Failed to initialize gldraw_init");
      return FALSE;
    }

  }

  res=gldraw_render (src->context, &src->gl_drawing, &src->audio_samples_buf.result);

  return res;

}



static void
_fill_gl (GstGLContext * context, GstGLSoundbar * src)
{
  src->gl_result = gst_gl_framebuffer_draw_to_texture (src->fbo, src->out_tex,
      gst_gl_test_src_callback, src);
}



static GstFlowReturn
gst_gl_test_src_fill (GstGLSoundbar * psrc, GstBuffer * buffer)
{
  GstGLSoundbar *src = psrc;

  GstClockTime next_time;
  GstVideoFrame out_frame;
  GstGLSyncMeta *sync_meta;

  if (!gst_video_frame_map (&out_frame, &src->vinfo, buffer,
          GST_MAP_WRITE | GST_MAP_GL)) {
    return GST_FLOW_NOT_NEGOTIATED;
  }

  src->out_tex = (GstGLMemory *) out_frame.map[0].memory;

  gst_gl_context_thread_add (src->context, (GstGLContextThreadFunc) _fill_gl, src);

  if (!src->gl_result) {
    gst_video_frame_unmap (&out_frame);
    goto gl_error;
  }
  gst_video_frame_unmap (&out_frame);
  if (!src->gl_result)
    goto gl_error;

  sync_meta = gst_buffer_get_gl_sync_meta (buffer);
  if (sync_meta)
    gst_gl_sync_meta_set_sync_point (sync_meta, src->context);

  src->n_frames++;
  next_time = gst_util_uint64_scale_int (src->n_frames * GST_SECOND,
        src->vinfo.fps_d, src->vinfo.fps_n);
  src->running_time = next_time;

  return GST_FLOW_OK;

gl_error:
  {
    return GST_FLOW_NOT_NEGOTIATED;
  }

}


static GstFlowReturn
gst_glsoundbar_chain (GstPad * pad, GstObject * parent,
    GstBuffer * buffer)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstGLSoundbar *scope;

  GstBuffer *inbuf;
  guint64 dist, ts;
  guint avail, sbpf;
  gpointer adata;
  gint bpf, rate;

  scope = GST_GLSOUNDBAR (parent);

  GST_LOG_OBJECT (scope, "chainfunc called");

  // resync on DISCONT
  if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DISCONT)) {
    gst_adapter_clear (scope->priv->adapter);
  }

  // Make sure have an output format
  if (gst_pad_check_reconfigure (scope->priv->srcpad)) {
    if (!gst_glsoundbar_src_negotiate (scope)) {
      gst_pad_mark_reconfigure (scope->priv->srcpad);
      goto not_negotiated;
    }
  }

  rate = GST_AUDIO_INFO_RATE (&scope->ainfo);
  bpf = GST_AUDIO_INFO_BPF (&scope->ainfo);

  if (bpf == 0) {
    ret = GST_FLOW_NOT_NEGOTIATED;
    goto beach;
  }

  gst_adapter_push (scope->priv->adapter, buffer);

  g_mutex_lock (&scope->priv->config_lock);

  // this is what we want
  sbpf = scope->req_spf * bpf;

  inbuf = scope->priv->inbuf;
  // FIXME: the timestamp in the adapter would be different
  gst_buffer_copy_into (inbuf, buffer, GST_BUFFER_COPY_METADATA, 0, -1);

  // this is what we have
  avail = gst_adapter_available (scope->priv->adapter);
  GST_LOG_OBJECT (scope, "avail: %u, bpf: %u", avail, sbpf);
  while (avail >= sbpf) {
    GstBuffer *outbuf;
    //GstVideoFrame outframe;

    // get timestamp of the current adapter content
    ts = gst_adapter_prev_pts (scope->priv->adapter, &dist);
    if (GST_CLOCK_TIME_IS_VALID (ts)) {
      // convert bytes to time
      ts += gst_util_uint64_scale_int (dist, GST_SECOND, rate * bpf);
    }

    // check for QoS, don't compute buffers that are known to be late
    if (GST_CLOCK_TIME_IS_VALID (ts)) {
      GstClockTime earliest_time;
      gdouble proportion;
      gint64 qostime;

      qostime =
          gst_segment_to_running_time (&scope->priv->segment,
          GST_FORMAT_TIME, ts) + scope->priv->frame_duration;

      GST_OBJECT_LOCK (scope);
      earliest_time = scope->priv->earliest_time;
      proportion = scope->priv->proportion;
      GST_OBJECT_UNLOCK (scope);

      if (GST_CLOCK_TIME_IS_VALID (earliest_time) && qostime <= earliest_time) {
        GstClockTime stream_time, jitter;
        GstMessage *qos_msg;

        GST_DEBUG_OBJECT (scope,
            "QoS: skip ts: %" GST_TIME_FORMAT ", earliest: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (qostime), GST_TIME_ARGS (earliest_time));

        ++scope->priv->dropped;
        stream_time = gst_segment_to_stream_time (&scope->priv->segment,
            GST_FORMAT_TIME, ts);
        jitter = GST_CLOCK_DIFF (qostime, earliest_time);
        qos_msg = gst_message_new_qos (GST_OBJECT (scope), FALSE, qostime,
            stream_time, ts, GST_BUFFER_DURATION (buffer));
        gst_message_set_qos_values (qos_msg, jitter, proportion, 1000000);
        gst_message_set_qos_stats (qos_msg, GST_FORMAT_BUFFERS,
            scope->priv->processed, scope->priv->dropped);
        gst_element_post_message (GST_ELEMENT (scope), qos_msg);

        goto skip;
      }
    }

    ++scope->priv->processed;

    g_mutex_unlock (&scope->priv->config_lock);
    ret = default_prepare_output_buffer (scope, &outbuf);
    g_mutex_lock (&scope->priv->config_lock);
    // recheck as the value could have changed
    sbpf = scope->req_spf * bpf;

    // no buffer allocated, we don't care why.
    if (ret != GST_FLOW_OK)
      break;

    // sync controlled properties
    if (GST_CLOCK_TIME_IS_VALID (ts))
      gst_object_sync_values (GST_OBJECT (scope), ts);

    GST_BUFFER_PTS (outbuf) = ts;
    GST_BUFFER_DURATION (outbuf) = scope->priv->frame_duration;

    // this can fail as the data size we need could have changed
    if (!(adata = (gpointer) gst_adapter_map (scope->priv->adapter, sbpf)))
      break;

    gst_gl_test_src_fill(scope, outbuf);

    gst_buffer_replace_all_memory (inbuf,
        gst_memory_new_wrapped (GST_MEMORY_FLAG_READONLY, adata, sbpf, 0,
            sbpf, NULL, NULL));

    //get audio samples:
    GstMapInfo amap;

    gst_buffer_map (inbuf, &amap, GST_MAP_READ);

    if(!audiosamplesbuf_set_data(&scope->audio_samples_buf, &scope->ainfo,
                    (gpointer *)amap.data, amap.size / scope->ainfo.bpf)){
      goto audiosamplesbuf_set_data_error;
    }

    gst_buffer_unmap (inbuf, &amap);
    //get audio samples

    if(!audiosamplesbuf_proceed(&scope->audio_samples_buf)){
      goto audiosamplesbuf_proceed_error;
    }

    g_mutex_unlock (&scope->priv->config_lock);
    ret = gst_pad_push (scope->priv->srcpad, outbuf);
    outbuf = NULL;
    g_mutex_lock (&scope->priv->config_lock);

  skip:
    // recheck as the value could have changed
    sbpf = scope->req_spf * bpf;
    GST_LOG_OBJECT (scope, "avail: %u, bpf: %u", avail, sbpf);
    // we want to take less or more, depending on spf : req_spf
    if (avail - sbpf >= sbpf) {
      gst_adapter_flush (scope->priv->adapter, sbpf);
      gst_adapter_unmap (scope->priv->adapter);
    } else if (avail >= sbpf) {
      // just flush a bit and stop
      gst_adapter_flush (scope->priv->adapter, (avail - sbpf));
      gst_adapter_unmap (scope->priv->adapter);
      break;
    }
    avail = gst_adapter_available (scope->priv->adapter);

    if (ret != GST_FLOW_OK)
      break;
  }

  g_mutex_unlock (&scope->priv->config_lock);

beach:
  return ret;

  // ERRORS
not_negotiated:
  {
    GST_DEBUG_OBJECT (scope, "Failed to renegotiate");
    return GST_FLOW_NOT_NEGOTIATED;
  }
audiosamplesbuf_set_data_error:
  {
    GST_DEBUG_OBJECT (scope, "Failed to audiosamplesbuf_set_data");
    return GST_FLOW_NOT_NEGOTIATED;
  }
audiosamplesbuf_proceed_error:
  {
    GST_DEBUG_OBJECT (scope, "Failed to audiosamplesbuf_proceed");
    return GST_FLOW_NOT_NEGOTIATED;
  }

}



static gboolean
gst_glsoundbar_src_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res;
  GstGLSoundbar *scope;

  //breakpoint_01//
  scope = GST_GLSOUNDBAR (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_QOS:
    {
      gdouble proportion;
      GstClockTimeDiff diff;
      GstClockTime timestamp;

      gst_event_parse_qos (event, NULL, &proportion, &diff, &timestamp);

      // save stuff for the _chain() function
      GST_OBJECT_LOCK (scope);
      scope->priv->proportion = proportion;
      if (diff >= 0)
        // we're late, this is a good estimate for next displayable
        // frame (see part-qos.txt)
        scope->priv->earliest_time = timestamp + 2 * diff +
            scope->priv->frame_duration;
      else
        scope->priv->earliest_time = timestamp + diff;
      GST_OBJECT_UNLOCK (scope);

      res = gst_pad_push_event (scope->priv->sinkpad, event);
      break;
    }
    case GST_EVENT_RECONFIGURE:
      // dont't forward
      gst_event_unref (event);
      res = TRUE;
      break;
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static gboolean
gst_glsoundbar_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean res;
  GstGLSoundbar *scope;

  //breakpoint_01//
  scope = GST_GLSOUNDBAR (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      res = gst_glsoundbar_sink_setcaps (scope, caps);
      gst_event_unref (event);
      break;
    }
    case GST_EVENT_FLUSH_STOP:
      gst_glsoundbar_reset (scope);
      res = gst_pad_push_event (scope->priv->srcpad, event);
      break;
    case GST_EVENT_SEGMENT:
    {
      // the newsegment values are used to clip the input samples
      // and to convert the incomming timestamps to running time so
      // we can do QoS
      gst_event_copy_segment (event, &scope->priv->segment);

      res = gst_pad_push_event (scope->priv->srcpad, event);
      break;
    }
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static gboolean
gst_glsoundbar_src_query (GstPad * pad, GstObject * parent,
    GstQuery * query)
{
  gboolean res = FALSE;
  GstGLSoundbar *scope;

  //breakpoint_01//
  scope = GST_GLSOUNDBAR (parent);



  switch (GST_QUERY_TYPE (query)) {

    case GST_QUERY_LATENCY:
    {
      // We need to send the query upstream and add the returned latency to our
      // own
      GstClockTime min_latency, max_latency;
      gboolean us_live;
      GstClockTime our_latency;
      guint max_samples;
      gint rate = GST_AUDIO_INFO_RATE (&scope->ainfo);

      if (rate == 0)
        break;

      if ((res = gst_pad_peer_query (scope->priv->sinkpad, query))) {
        gst_query_parse_latency (query, &us_live, &min_latency, &max_latency);

        GST_DEBUG_OBJECT (scope, "Peer latency: min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min_latency), GST_TIME_ARGS (max_latency));

        // the max samples we must buffer buffer
        max_samples = MAX (scope->req_spf, scope->priv->spf);
        our_latency = gst_util_uint64_scale_int (max_samples, GST_SECOND, rate);

        GST_DEBUG_OBJECT (scope, "Our latency: %" GST_TIME_FORMAT,
            GST_TIME_ARGS (our_latency));

        // we add some latency but only if we need to buffer more than what
        // upstream gives us
        min_latency += our_latency;
        if (max_latency != -1)
          max_latency += our_latency;

        GST_DEBUG_OBJECT (scope, "Calculated total latency : min %"
            GST_TIME_FORMAT " max %" GST_TIME_FORMAT,
            GST_TIME_ARGS (min_latency), GST_TIME_ARGS (max_latency));

        gst_query_set_latency (query, TRUE, min_latency, max_latency);
      }
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }

  return res;
}

static GstStateChangeReturn
gst_glsoundbar_change_state (GstElement * element,
    GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstGLSoundbar *scope;

  //breakpoint_01//
  scope = GST_GLSOUNDBAR (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_glsoundbar_reset (scope);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_glsoundbar_set_allocation (scope, NULL, NULL, NULL, NULL);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}



static void
gst_glsoundbar_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{

  GstGLSoundbar *scope = GST_GLSOUNDBAR (object);

  switch (prop_id) {
    case PROP_BARS_DIRECTION:
        scope->bars_draw_direction=g_value_get_int(value);
      break;
    case PROP_BAR_ASPECT:
        scope->bar_aspect=g_value_get_float(value);
      break;
    case PROP_BAR_RISC_LEN:
        scope->bar_risc_len=g_value_get_float(value);
      break;
    case PROP_BAR_RISC_STEP:
        scope->bar_risc_step=g_value_get_float(value);
      break;
    case PROP_TIMESTAMP_OFFSET:
        scope->timestamp_offset = g_value_get_int64 (value);
      break;
    case PROP_PEAK_HEIGHT_PIXELS:
        scope->peak_height=g_value_get_float(value);
      break;

    case PROP_AUDIO_LOUD_SPEED:
        scope->audio_loud_speed=g_value_get_float(value);
      break;
    case PROP_AUDIO_PEAK_SPEED:
        scope->audio_peak_speed=g_value_get_float(value);
      break;
    case PROP_BAR_ASPECT_AUTO:
        scope->bar_aspect_auto=g_value_get_float(value);
      break;
    case PROP_BG_COLOR_R:
        scope->bg_color_r=g_value_get_float(value);
      break;
    case PROP_BG_COLOR_G:
        scope->bg_color_g=g_value_get_float(value);
      break;
    case PROP_BG_COLOR_B:
        scope->bg_color_b=g_value_get_float(value);
      break;
    case PROP_BG_COLOR_A:
        scope->bg_color_a=g_value_get_float(value);
      break;


    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

}

static void
gst_glsoundbar_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{

  GstGLSoundbar *scope = GST_GLSOUNDBAR (object);

  switch (prop_id) {
    case PROP_BARS_DIRECTION:
        g_value_set_int(value,scope->bars_draw_direction);
      break;
    case PROP_BAR_ASPECT:
        g_value_set_float(value,scope->bar_aspect);
      break;
    case PROP_BAR_RISC_LEN:
        g_value_set_float(value,scope->bar_risc_len);
      break;
    case PROP_BAR_RISC_STEP:
        g_value_set_float(value,scope->bar_risc_step);
      break;
    case PROP_TIMESTAMP_OFFSET:
        g_value_set_int64(value, scope->timestamp_offset);
      break;
    case PROP_PEAK_HEIGHT_PIXELS:
        g_value_set_float(value, scope->peak_height);
      break;

    case PROP_AUDIO_LOUD_SPEED:
        g_value_set_float(value, scope->audio_loud_speed);
      break;
    case PROP_AUDIO_PEAK_SPEED:
        g_value_set_float(value, scope->audio_peak_speed);
      break;
    case PROP_BAR_ASPECT_AUTO:
        g_value_set_int(value, scope->bar_aspect_auto);
      break;
    case PROP_BG_COLOR_R:
        g_value_set_float(value, scope->bg_color_r);
      break;
    case PROP_BG_COLOR_G:
        g_value_set_float(value, scope->bg_color_g);
      break;
    case PROP_BG_COLOR_B:
        g_value_set_float(value, scope->bg_color_b);
      break;
    case PROP_BG_COLOR_A:
        g_value_set_float(value, scope->bg_color_a);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_glsoundbar_class_init (GstGLSoundbarClass * klass)
{

  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gst_element_class_set_details_simple(gstelement_class,
    "GLSoundbarBase",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "NIITV Ivan Fedotov<ivanfedotovmail@yandex.ru");

  klass->supported_gl_api = GST_GL_API_ANY;

  g_type_class_add_private (klass, sizeof (GstGLSoundbarPrivate));

  gobject_class->set_property = gst_glsoundbar_set_property;
  gobject_class->get_property = gst_glsoundbar_get_property;
  gobject_class->dispose = gst_glsoundbar_dispose;

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_glsoundbar_change_state);

  gobject_class->finalize = gst_glsoundbar_finalize;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_glsoundbar_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_glsoundbar_sink_template);


  g_object_class_install_property
        (gobject_class, PROP_BARS_DIRECTION,
         g_param_spec_int("direction", "Bar draw direction",
                          "0 - direction to up, 1 - direction to right",
                          0, 1, 0,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_BAR_ASPECT,
         g_param_spec_float("bar-aspect", "Bar draw aspect",
                          "Aspect = width/height for to up direction, another directions same value, its calc automatic",
                          0.0, 1000000.0, 0.05,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_BAR_RISC_LEN,
         g_param_spec_float("bar-risc-length-percent", "Bar risc length percent",
                          "Bar risc length percent at bar height",
                          0, 100000, 0.022,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_BAR_RISC_STEP,
         g_param_spec_float("bar-risc-step-percent", "Bar risc step",
                          "Bar risc length percent at bar height",
                          0, 100000, 0.028,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_PEAK_HEIGHT_PIXELS,
         g_param_spec_float("peak-height-percent", "Peak height percent",
                          "Bar risc length percent at bar height",
                          0, 100000, 0.02,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_AUDIO_LOUD_SPEED,
         g_param_spec_float("audio-loud-speed", "Audio loud speed",
                          "Audio loud average calculation speed",
                          0.001, 1000.0, 1.0,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_AUDIO_PEAK_SPEED,
         g_param_spec_float("audio-peak-speed", "Audio peak speed",
                          "Audio peak fall speed",
                          0.001, 1000.0, 1.0,
                          G_PARAM_READWRITE));

     g_object_class_install_property
        (gobject_class, PROP_BAR_ASPECT_AUTO,
         g_param_spec_int("bar-aspect-auto", "Bar auto aspect",
                          "0 - auto aspect disable, 1 - auto aspect enable",
                          0, 1, 1,
                          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_TIMESTAMP_OFFSET, g_param_spec_int64 ("timestamp-offset",
          "Timestamp offset",
          "An offset added to timestamps set on buffers (in ns)", G_MININT64,
          G_MAXINT64, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property
        (gobject_class, PROP_BG_COLOR_R,
         g_param_spec_float("bg-color-r", "Backgound color R",
                          "Backgound color R",
                          0.0, 1.0, 1.0,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_BG_COLOR_G,
         g_param_spec_float("bg-color-g", "Backgound color G",
                          "Backgound color G",
                          0.0, 1.0, 1.0,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_BG_COLOR_B,
         g_param_spec_float("bg-color-b", "Backgound color B",
                          "Backgound color B",
                          0.0, 1.0, 1.0,
                          G_PARAM_READWRITE));

  g_object_class_install_property
        (gobject_class, PROP_BG_COLOR_A,
         g_param_spec_float("bg-color-a", "Backgound color A",
                          "Backgound color A",
                          0.0, 1.0, 1.0,
                          G_PARAM_READWRITE));

}


static void
gst_glsoundbar_init (GstGLSoundbar * filter)
{
  GstPadTemplate *pad_template;

  GstGLSoundbarClass *filter_class = GST_GLSOUNDBAR_CLASS (G_OBJECT_GET_CLASS (filter));

  filter->priv = GST_GLSOUNDBAR_GET_PRIVATE (filter);

  audiosamplesbuf_init(&filter->audio_samples_buf);

  // create the sink and src pads
  pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_CLASS (filter_class), "sink");
  g_return_if_fail (pad_template != NULL);
  filter->priv->sinkpad = gst_pad_new_from_template (pad_template, "sink");
  gst_pad_set_chain_function (filter->priv->sinkpad,
      GST_DEBUG_FUNCPTR (gst_glsoundbar_chain));
  gst_pad_set_event_function (filter->priv->sinkpad,
      GST_DEBUG_FUNCPTR (gst_glsoundbar_sink_event));
  gst_element_add_pad (GST_ELEMENT (filter), filter->priv->sinkpad);

  pad_template =
      gst_element_class_get_pad_template (GST_ELEMENT_CLASS (filter_class), "src");
  g_return_if_fail (pad_template != NULL);
  filter->priv->srcpad = gst_pad_new_from_template (pad_template, "src");
  gst_pad_set_event_function (filter->priv->srcpad,
      GST_DEBUG_FUNCPTR (gst_glsoundbar_src_event));
  gst_pad_set_query_function (filter->priv->srcpad,
      GST_DEBUG_FUNCPTR (gst_glsoundbar_src_query));
  gst_element_add_pad (GST_ELEMENT (filter), filter->priv->srcpad);

  filter->priv->adapter = gst_adapter_new ();
  filter->priv->inbuf = gst_buffer_new ();

  // reset the initial video state
  gst_video_info_init (&filter->vinfo);
  filter->priv->frame_duration = GST_CLOCK_TIME_NONE;

  // reset the initial state
  gst_audio_info_init (&filter->ainfo);
  gst_video_info_init (&filter->vinfo);
  g_mutex_init(&filter->priv->config_lock);

  filter->gl_drawing_created=FALSE;

  filter->fbo=NULL;
  filter->out_tex=NULL;
  filter->display=NULL;
  filter->context=NULL;
  filter->other_context=NULL;
  filter->gl_result=FALSE;

  filter->src_impl=NULL;
  filter->running_time = 0;
  filter->timestamp_offset = 0;
  filter->n_frames = 0;

  filter->current_time=time(0)*1000;
  filter->prev_time=filter->current_time;

  g_get_current_time(&filter->gl_draw_timeprev);
  g_get_current_time(&filter->gl_draw_timecurr);

  filter->bars_draw_direction=GLSOUND_BAR_DRAW_DIRECTION_TO_UP;
  filter->bar_aspect=0.05;
  filter->bar_risc_len=0.022;
  filter->bar_risc_step=0.028;
  filter->peak_height=0.015;

  filter->audio_loud_speed=1.0;
  filter->audio_peak_speed=1.0;
  filter->bar_aspect_auto=1;

  filter->bg_color_r=0.0;
  filter->bg_color_g=0.0;
  filter->bg_color_b=0.0;
  filter->bg_color_a=1.0;

}

static gboolean
glsoundbar_init (GstPlugin * glsoundbar)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template glsoundbar' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_glsoundbar_debug, "glsoundbar",
      0, "Template glsoundbar");

  return gst_element_register (glsoundbar, "glsoundbar", GST_RANK_NONE,
      GST_TYPE_GLSOUNDBAR);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstglsoundbar"
#endif

/* gstreamer looks for this structure to register glsoundbars
 *
 * exchange the string 'Template glsoundbar' with your glsoundbar description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    glsoundbar,
    "Template glsoundbar",
    glsoundbar_init,
    "1.0",
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)

