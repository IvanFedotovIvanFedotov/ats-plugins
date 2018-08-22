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
			 GST_TYPE_VIDEO_FILTER,
			 GST_DEBUG_CATEGORY_INIT (gst_videoanalysis_debug_category,
						  "videoanalysis", 0,
						  "debug category for videoanalysis element"));

static void
gst_videoanalysis_class_init (GstVideoAnalysisClass * klass)
{
        GObjectClass *gobject_class = (GObjectClass *) klass;
        GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
        GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);

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


}

static void
gst_videoanalysis_init (GstVideoAnalysis *videoanalysis)
{

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
