/*
 * TODO copyright
 */

#ifndef LGPL_LIC
#define LIC "Proprietary"
#define URL "http://www.niitv.ru/"
#else
#define LIC "LGPL"
#define URL "https://github.com/Freyr666/ats-analyzer/"
#endif

/**
 * SECTION:element-videoanalysis
 * @title: videoanalysis
 *
 * QoE analysis based on shaders.
 *
 * ## Examples
 * |[
 * gst-launch-1.0 videotestsrc ! glupload ! gstvideoanalysis ! glimagesink
 * ]|
 * FBO (Frame Buffer Object) and GLSL (OpenGL Shading Language) are required.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gl/gstglfuncs.h>

#include "gstvideoanalysis.h"
#include "analysis.h"

#define GST_CAT_DEFAULT gst_videoanalysis_debug_category
GST_DEBUG_CATEGORY_STATIC (gst_videoanalysis_debug_category);

#define gst_videoanalysis_parent_class parent_class

static gboolean gst_videoanalysis_start (GstBaseTransform * trans);
static gboolean gst_videoanalysis_stop  (GstBaseTransform * trans);
static GstFlowReturn gst_videoanalysis_transform_ip (GstBaseTransform * filter,
                                                     GstBuffer * inbuf);
static GstFlowReturn gst_videoanalysis_prepare_output_buffer (GstBaseTransform * filter,
                                                              GstBuffer * inbuf,
                                                              GstBuffer ** outbuf);
static gboolean gst_videoanalysis_set_caps (GstBaseTransform * trans,
                                            GstCaps * incaps,
                                            GstCaps * outcaps);
static gboolean gst_videoanalysis_gl_start (GstGLBaseFilter * trans);
static gboolean gst_videoanalysis_gl_set_caps (GstGLBaseFilter * trans,
                                               GstCaps * incaps,
                                               GstCaps * outcaps);
static gboolean videoanalysis_apply (GstVideoAnalysis * va, GstGLMemory * mem);
//static gboolean gst_gl_base_filter_find_gl_context (GstGLBaseFilter * filter);

/* signals
enum
{
        DATA_SIGNAL,
        LAST_SIGNAL
};
*/

/* args */
enum
{
        PROP_0,
        LAST_PROP
};

/*static guint      signals[LAST_SIGNAL]   = { 0 };*/
static GParamSpec *properties[LAST_PROP] = { NULL, };

/* pad templates */
static const gchar caps_string[] =
        "video/x-raw(memory:GLMemory),format=(string){I420,NV12,NV21,YV12,IYUV}";

/* class initialization */
G_DEFINE_TYPE_WITH_CODE (GstVideoAnalysis,
			 gst_videoanalysis,
			 GST_TYPE_GL_BASE_FILTER,
			 GST_DEBUG_CATEGORY_INIT (gst_videoanalysis_debug_category,
						  "videoanalysis", 0,
						  "debug category for videoanalysis element"));

static void
gst_videoanalysis_class_init (GstVideoAnalysisClass * klass)
{
        //GObjectClass *gobject_class = (GObjectClass *) klass;
        GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
        GstGLBaseFilterClass *base_filter = GST_GL_BASE_FILTER_CLASS(klass);

        /* Setting up pads and setting metadata should be moved to
           base_class_init if you intend to subclass this class. */
        gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
                                            gst_pad_template_new ("sink",
                                                                  GST_PAD_SINK,
                                                                  GST_PAD_ALWAYS,
                                                                  gst_caps_from_string (caps_string)));
        gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
                                            gst_pad_template_new ("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  gst_caps_from_string (caps_string)));

        gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
                                               "Gstreamer element for video analysis",
                                               "Video data analysis",
                                               "filter for video analysis",
                                               "freyr <sky_rider_93@mail.ru>");

        base_transform_class->passthrough_on_same_caps = TRUE;
        base_transform_class->transform_ip_on_passthrough = TRUE;
        base_transform_class->start = gst_videoanalysis_start;
        base_transform_class->stop = gst_videoanalysis_stop;
        base_transform_class->transform_ip = gst_videoanalysis_transform_ip;
        //base_transform_class->prepare_output_buffer = gst_videoanalysis_prepare_output_buffer;
        base_transform_class->set_caps = gst_videoanalysis_set_caps;
        //base_filter->gl_set_caps = gst_videoanalysis_gl_set_caps;
        base_filter->gl_start = gst_videoanalysis_gl_start;
        base_filter->supported_gl_api =
                GST_GL_API_OPENGL | GST_GL_API_GLES2 | GST_GL_API_OPENGL3;
}

static void
gst_videoanalysis_init (GstVideoAnalysis *videoanalysis)
{
        videoanalysis->fbo = NULL;
        videoanalysis->shader = NULL;
        videoanalysis->prev_buffer = NULL;
        videoanalysis->prev_tex = NULL;
}

static gboolean
gst_videoanalysis_start (GstBaseTransform * trans)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);
        
        return GST_BASE_TRANSFORM_CLASS(parent_class)->start(trans);
}

static gboolean
gst_videoanalysis_stop (GstBaseTransform * trans)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);
        
        return GST_BASE_TRANSFORM_CLASS(parent_class)->stop(trans);
}

static void
shader_create (GstGLContext * context, GstVideoAnalysis * va)
{
        GError* error = NULL;
        if (!(va->shader =
              gst_gl_shader_new_link_with_stages(context, &error,
                                                 gst_glsl_stage_new_default_vertex(context),
                                                 gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
                                                                                 GST_GLSL_VERSION_450,
                                                                                 GST_GLSL_PROFILE_CORE
                                                                                 | GST_GLSL_PROFILE_ES,
                                                                                 fragment_source),
                                                 NULL))) {
                GST_ELEMENT_ERROR (va, RESOURCE, NOT_FOUND,
                                   ("Failed to initialize shader"), (NULL));
        }
}

static gboolean
gst_videoanalysis_set_caps (GstBaseTransform * trans,
                            GstCaps * incaps,
                            GstCaps * outcaps)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);

        if (!gst_video_info_from_caps (&videoanalysis->in_info, incaps))
                goto wrong_caps;
        if (!gst_video_info_from_caps (&videoanalysis->out_info, outcaps))
                goto wrong_caps;

        //gst_gl_base_filter_find_gl_context (GST_GL_BASE_FILTER(trans));
        //gst_gl_context_thread_add(GST_GL_BASE_FILTER(trans)->context, shader_create, videoanalysis);
        
        return GST_BASE_TRANSFORM_CLASS(parent_class)->set_caps(trans,incaps,outcaps);

        /* ERRORS */
wrong_caps:
        GST_WARNING ("Wrong caps - could not understand input or output caps");
        return FALSE;
}

static gboolean
gst_videoanalysis_gl_start (GstGLBaseFilter * trans)
{
        GError* error = NULL;
        GstVideoAnalysis *va = GST_VIDEOANALYSIS (trans);
        //gst_gl_base_filter_find_gl_context (GST_GL_BASE_FILTER(va));
        GstGLContext *context = GST_GL_BASE_FILTER (va)->context;
        if (!(va->shader =
              gst_gl_shader_new_link_with_stages(context, &error,
                                                 gst_glsl_stage_new_default_vertex(context),
                                                 gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
                                                                                 GST_GLSL_VERSION_450,
                                                                                 GST_GLSL_PROFILE_CORE
                                                                                 | GST_GLSL_PROFILE_ES,
                                                                                 fragment_source),
                                                 NULL))) {
                GST_ELEMENT_ERROR (va, RESOURCE, NOT_FOUND,
                                   ("Failed to initialize shader"), (NULL));
        }
        return TRUE;
}
        

static gboolean
gst_videoanalysis_gl_set_caps (GstGLBaseFilter * trans,
                           GstCaps * incaps,
                           GstCaps * outcaps)
{
        GError* error = NULL;
        GstVideoAnalysis *va = GST_VIDEOANALYSIS (trans);
        // GstGLFilterClass *va_class = GST_GL_FILTER_GET_CLASS (filter);
        GstGLContext *context = GST_GL_BASE_FILTER (va)->context;
        gint in_width, in_height;

        in_width = GST_VIDEO_INFO_WIDTH (&va->in_info);
        in_height = GST_VIDEO_INFO_HEIGHT (&va->in_info);

        if (va->fbo)
                gst_object_unref (va->fbo);

        if (!(va->fbo =
              gst_gl_framebuffer_new_with_default_depth (context,
                                                         in_width,
                                                         in_height)))
                goto context_error;
        /*
        if (filter_class->init_fbo) {
                if (!filter_class->init_fbo (filter))
                        goto error;
        }
        */

        return TRUE;

context_error:
        GST_ELEMENT_ERROR (va, RESOURCE, NOT_FOUND, ("Could not generate FBO"),
                           (NULL));
        return FALSE;
error:
        GST_ELEMENT_ERROR (va, LIBRARY, INIT,
                           ("Subclass failed to initialize."), (NULL));
        return FALSE;
}
/*
static GstFlowReturn gst_videoanalysis_prepare_output_buffer (GstBaseTransform * filter,
                                                              GstBuffer * inbuf,
                                                              GstBuffer ** outbuf)
{
        *outbuf = inbuf;
        return GST_FLOW_OK;
}
*/
static GstFlowReturn
gst_videoanalysis_transform_ip (GstBaseTransform * trans,
                                GstBuffer * inbuf)
{
        GstMemory *tex, *out_tex;
        GstVideoFrame gl_frame, out_frame;
        gboolean ret = GST_FLOW_OK;

        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);
        if (!gst_video_frame_map (&gl_frame, &videoanalysis->in_info, inbuf,
                                  GST_MAP_READ | GST_MAP_GL)) {
                ret = GST_FLOW_ERROR;
                goto inbuf_error;
        }
        
        /* map[0] corresponds to the Y component of Yuv */
        tex = gl_frame.map[0].memory;
        if (!gst_is_gl_memory (tex)) {
                ret = GST_FLOW_ERROR;
                GST_ERROR_OBJECT (videoanalysis, "Input memory must be GstGLMemory");
                goto unmap_error;
        }

        videoanalysis_apply (videoanalysis, GST_GL_MEMORY_CAST (tex));
        
        gst_buffer_replace (&videoanalysis->prev_buffer, inbuf);
                
unmap_error:
        gst_video_frame_unmap (&gl_frame);
inbuf_error:
        return GST_FLOW_OK;
}

struct glcb {
        GstVideoAnalysis * va;
        GstGLMemory      * tex;
};

static void
analyse (GstGLContext *context, struct glcb * cb)
{
        GstVideoAnalysis * va = cb->va;
        GstGLMemory      * tex = cb->tex;
        const GstGLFuncs *gl = context->gl_vtable;

        if (! va->shader )
                shader_create(context, va);
        
        gst_gl_shader_use (va->shader);

        gl->ActiveTexture (GL_TEXTURE0);
        gl->BindTexture (GL_TEXTURE_2D, gst_gl_memory_get_texture_id (tex));

        gst_gl_shader_set_uniform_1i (va->shader, "tex", 0);
        gst_gl_shader_set_uniform_1f (va->shader, "width",
                                      GST_VIDEO_INFO_WIDTH (&va->in_info));
        gst_gl_shader_set_uniform_1f (va->shader, "height",
                                      GST_VIDEO_INFO_HEIGHT (&va->in_info));

        //gst_gl_framebuffer_draw_to_texture(va->fbo, out_tex, draw, NULL);

        va->prev_tex = tex;
}

static gboolean
videoanalysis_apply (GstVideoAnalysis * va, GstGLMemory * tex)
{
        struct glcb cb;
        GstGLSyncMeta *out_sync_meta, *in_sync_meta;
        GstGLContext *context = GST_GL_BASE_FILTER (va)->context;

        cb.va = va;
        cb.tex = tex;
        gst_gl_context_thread_add(context, (GstGLContextThreadFunc) analyse, &cb);
        return TRUE;
}
   
static gboolean
plugin_init (GstPlugin * plugin)
{

        /* FIXME Remember to set the rank if it's an element that is meant
           to be autoplugged by decodebin. */
        return gst_element_register (plugin,
                                     "videoanalysis",
                                     GST_RANK_NONE,
                                     GST_TYPE_VIDEOANALYSIS);
}

/*
static void
gst_gl_base_filter_gl_start (GstGLContext * context, gpointer data)
{
        GstGLBaseFilter *filter = GST_GL_BASE_FILTER (data);
        GstGLBaseFilterClass *filter_class = GST_GL_BASE_FILTER_GET_CLASS (filter);

        gst_gl_insert_debug_marker (filter->context,
                                    "starting element %s", GST_OBJECT_NAME (filter));

        filter->priv->gl_started = filter_class->gl_start (filter);
}


static void
gst_gl_base_filter_gl_stop (GstGLContext * context, gpointer data)
{
        GstGLBaseFilter *filter = GST_GL_BASE_FILTER (data);
        GstGLBaseFilterClass *filter_class = GST_GL_BASE_FILTER_GET_CLASS (filter);

        gst_gl_insert_debug_marker (filter->context,
                                    "stopping element %s", GST_OBJECT_NAME (filter));

        if (filter->priv->gl_started)
                filter_class->gl_stop (filter);

        filter->priv->gl_started = FALSE;
}

gboolean
gst_gl_base_filter_find_gl_context (GstGLBaseFilter * filter)
{
        GstGLBaseFilterClass *filter_class = GST_GL_BASE_FILTER_GET_CLASS (filter);
        GError *error = NULL;
        gboolean new_context = FALSE;

        if (!filter->context)
                new_context = TRUE;

        _find_local_gl_context (filter);

        if (!filter->context) {
                GST_OBJECT_LOCK (filter->display);
                do {
                        if (filter->context)
                                gst_object_unref (filter->context);
                        / just get a GL context.  we don't care /
                        filter->context =
                                gst_gl_display_get_gl_context_for_thread (filter->display, NULL);
                        if (!filter->context) {
                                if (!gst_gl_display_create_context (filter->display,
                                                                    filter->priv->other_context, &filter->context, &error)) {
                                        GST_OBJECT_UNLOCK (filter->display);
                                        goto context_error;
                                }
                        }
                } while (!gst_gl_display_add_context (filter->display, filter->context));
                GST_OBJECT_UNLOCK (filter->display);
        }

        if (new_context || !filter->priv->gl_started) {
                if (filter->priv->gl_started)
                        gst_gl_context_thread_add (filter->context, gst_gl_base_filter_gl_stop,
                                                   filter);

                {
                        GstGLAPI current_gl_api = gst_gl_context_get_gl_api (filter->context);
                        if ((current_gl_api & filter_class->supported_gl_api) == 0)
                                goto unsupported_gl_api;
                }

                gst_gl_context_thread_add (filter->context, gst_gl_base_filter_gl_start,
                                           filter);

                if (!filter->priv->gl_started)
                        goto error;
        }

        return TRUE;

unsupported_gl_api:
        {
                GstGLAPI gl_api = gst_gl_context_get_gl_api (filter->context);
                gchar *gl_api_str = gst_gl_api_to_string (gl_api);
                gchar *supported_gl_api_str =
                        gst_gl_api_to_string (filter_class->supported_gl_api);

                GST_ELEMENT_ERROR (filter, RESOURCE, BUSY,
                                   ("GL API's not compatible context: %s supported: %s", gl_api_str,
                                    supported_gl_api_str), (NULL));

                g_free (supported_gl_api_str);
                g_free (gl_api_str);
                return FALSE;
        }
context_error:
        {
                GST_ELEMENT_ERROR (filter, RESOURCE, NOT_FOUND, ("%s", error->message),
                                   (NULL));
                g_clear_error (&error);
                return FALSE;
        }
error:
        {
                GST_ELEMENT_ERROR (filter, LIBRARY, INIT,
                                   ("Subclass failed to initialize."), (NULL));
                return FALSE;
        }
}
*/

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.1.9"
#endif
#ifndef PACKAGE
#define PACKAGE "videoanalysis"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "videoanalysis_package"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN URL
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		   GST_VERSION_MINOR,
		   videoanalysis,
		   "Package for video data analysis",
		   plugin_init, VERSION, LIC, PACKAGE_NAME, GST_PACKAGE_ORIGIN)
