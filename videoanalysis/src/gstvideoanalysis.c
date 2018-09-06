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
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLES3/gl31.h>
#include <assert.h>

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
        "video/x-raw(memory:GLMemory),format=(string){I420,NV12,NV21,YV12}";

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
        base_transform_class->set_caps = gst_videoanalysis_set_caps;
        base_filter->supported_gl_api = GST_GL_API_OPENGL3;
}

static void
gst_videoanalysis_init (GstVideoAnalysis *videoanalysis)
{
        videoanalysis->shader = NULL;
        videoanalysis->shader_auxilary = NULL;
        videoanalysis->tex = NULL;
        videoanalysis->prev_buffer = NULL;
        videoanalysis->prev_tex = NULL;
        videoanalysis->gl_settings_unchecked = TRUE;
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

        videoanalysis->tex = NULL;
        gst_buffer_replace(&videoanalysis->prev_buffer, NULL);
        videoanalysis->prev_tex = NULL;
        
        return GST_BASE_TRANSFORM_CLASS(parent_class)->stop(trans);
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

        gst_object_replace((GstObject**)&videoanalysis->shader, NULL);
        gst_object_replace((GstObject**)&videoanalysis->shader_auxilary, NULL);
        
        return GST_BASE_TRANSFORM_CLASS(parent_class)->set_caps(trans,incaps,outcaps);

        /* ERRORS */
wrong_caps:
        GST_WARNING ("Wrong caps - could not understand input or output caps");
        return FALSE;
}

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
/*
static void
fbo_create (GstGLContext * context, GstVideoAnalysis * va)
{
        gint in_width, in_height;
        in_width = GST_VIDEO_INFO_WIDTH (&va->in_info);
        in_height = GST_VIDEO_INFO_HEIGHT (&va->in_info);
        va->fbo = gst_gl_framebuffer_new_with_default_depth (context,
                                                             in_width,
                                                             in_height);
}
*/

static void
shader_create (GstGLContext * context, GstVideoAnalysis * va)
{
        GError * error;
        if (!(va->shader =
              gst_gl_shader_new_link_with_stages(context, &error,
                                                 gst_glsl_stage_new_with_string (context, GL_COMPUTE_SHADER,
                                                                                 GST_GLSL_VERSION_450,
                                                                                 GST_GLSL_PROFILE_CORE,
                                                                                 shader_source),
                                                 NULL))) {
                GST_ELEMENT_ERROR (va, RESOURCE, NOT_FOUND,
                                   ("Failed to initialize shader"), (NULL));
        }

        if (!(va->shader_auxilary =
              gst_gl_shader_new_link_with_stages(context, &error,
                                                 gst_glsl_stage_new_with_string (context, GL_COMPUTE_SHADER,
                                                                                 GST_GLSL_VERSION_450,
                                                                                 GST_GLSL_PROFILE_CORE,
                                                                                 shader_aux_source),
                                                 NULL))) {
                GST_ELEMENT_ERROR (va, RESOURCE, NOT_FOUND,
                                   ("Failed to initialize auxilary shader"), (NULL));
        }
}


static void
analyse (GstGLContext *context, GstVideoAnalysis * va)
{
        const GstGLFuncs * gl = context->gl_vtable;
        int width = va->in_info.width;
        int height = va->in_info.height;
        int stride = va->in_info.stride[0];
        GLuint aux_buffer, buffer;
        float* data;
        glGetError();

        if (G_LIKELY(va->prev_tex)) {
                gl->ActiveTexture (GL_TEXTURE0 + 1);
                gl->BindTexture (GL_TEXTURE_2D, gst_gl_memory_get_texture_id (va->prev_tex));
                glBindImageTexture(0, gst_gl_memory_get_texture_id (va->prev_tex),
                                   0, GL_FALSE, 0, GL_READ_WRITE, GL_R8);
        }

        g_printf("Texture: width %d, width %d, stride %d\n",
                 gst_gl_memory_get_texture_width(va->tex),
                 width,
                 stride);  
      
        gl->ActiveTexture (GL_TEXTURE0);      
        gl->BindTexture (GL_TEXTURE_2D, gst_gl_memory_get_texture_id (va->tex));
        glBindImageTexture(0, gst_gl_memory_get_texture_id (va->tex),
                           0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);

        glGenBuffers(1, &aux_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, aux_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (width / 8) * (height / 8) * sizeof(struct Accumulator),
                     NULL, GL_DYNAMIC_COPY);
        glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 10, aux_buffer);
        
        gl->GenBuffers(1, &buffer);
        gl->BindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
        gl->BufferData(GL_SHADER_STORAGE_BUFFER, 5 * sizeof(GLfloat), NULL, GL_DYNAMIC_COPY);
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 11, buffer, 0, sizeof(GLfloat) * 5);

        gst_gl_shader_use (va->shader);        

        GLuint prev_ind = va->prev_tex != 0 ? 1 : 0;
        gst_gl_shader_set_uniform_1i(va->shader, "tex", 0);
        gst_gl_shader_set_uniform_1i(va->shader, "tex_prev", prev_ind);
        gst_gl_shader_set_uniform_1i(va->shader, "width", width);
        gst_gl_shader_set_uniform_1i(va->shader, "height", height);
        gst_gl_shader_set_uniform_1i(va->shader, "stride", stride);
        gst_gl_shader_set_uniform_1i(va->shader, "black_bound", 16);
        gst_gl_shader_set_uniform_1i(va->shader, "freez_bound", 16);

        glDispatchCompute(width / 8, height / 8, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        
        gst_gl_shader_use (va->shader_auxilary);

        gst_gl_shader_set_uniform_1i(va->shader_auxilary, "width", width);
        gst_gl_shader_set_uniform_1i(va->shader_auxilary, "height", height);

        glDispatchCompute(width / 8, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        gl->Finish();

        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, buffer, 0, sizeof(GLfloat) * 5);
        data = (float *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLfloat) * 5, GL_MAP_READ_BIT);

        g_printf ("Shader Results: [block: %f; luma: %f; black: %f; diff: %f; freeze: %f]\n",
                  data[0], data[1], data[2], data[3], data[4]);

        /* Cleanup */
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        va->prev_tex = va->tex;
}

static void
_check_defaults_ (GstGLContext *context, GstVideoAnalysis * va) {
        const GstGLFuncs * gl = context->gl_vtable;
        int size, type;
        
        glGetInternalformativ(GL_TEXTURE_2D, GL_RED, GL_INTERNALFORMAT_RED_SIZE, 1, &size);
        glGetInternalformativ(GL_TEXTURE_2D, GL_RED, GL_INTERNALFORMAT_RED_TYPE, 1, &type);

        if (size == 8 && type == GL_UNSIGNED_NORMALIZED) {
                va->gl_settings_unchecked = FALSE;
        } else {
                GST_ERROR("Platform default red representation is not GL_R8");
                exit(-1);
        }
}

static gboolean
videoanalysis_apply (GstVideoAnalysis * va, GstGLMemory * tex)
{
        GstGLContext *context = GST_GL_BASE_FILTER (va)->context;

        /* Check system defaults */
        if (G_UNLIKELY(va->gl_settings_unchecked)) {
                gst_gl_context_thread_add(context, (GstGLContextThreadFunc) _check_defaults_, va);
        }
        /* Check texture format */
        if (G_UNLIKELY(gst_gl_memory_get_texture_format(tex) != GST_GL_RED)) {
                GST_ERROR("GL texture format should be GL_RED");
                exit(-1);
        }

        va->tex = tex;

        /* */
        /* Compile shader */
        if (! va->shader )
                gst_gl_context_thread_add(context, (GstGLContextThreadFunc) shader_create, va);

        gst_gl_context_thread_add(context, (GstGLContextThreadFunc) analyse, va);

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
