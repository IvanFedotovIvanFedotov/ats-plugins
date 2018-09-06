/*
 * TODO copyright
 */

#ifndef _GSTVIDEOANALYSIS_
#define _GSTVIDEOANALYSIS_

#include <gst/gl/gl.h>
#include <gst/gl/gstglbasefilter.h>
#include <GL/gl.h>

#include "videodata.h"
#include "error.h"

G_BEGIN_DECLS

GType gst_videoanalysis_get_type (void);
#define GST_TYPE_VIDEOANALYSIS                  \
        (gst_videoanalysis_get_type())
#define GST_VIDEOANALYSIS(obj)                                          \
        (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIDEOANALYSIS,GstVideoAnalysis))
#define GST_VIDEOANALYSIS_CLASS(klass)                                  \
        (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIDEOANALYSIS,GstVideoAnalysisClass))
#define GST_IS_VIDEOANALYSIS(obj)                                       \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIDEOANALYSIS))
#define GST_IS_VIDEOANALYSIS_CLASS(obj)                                 \
        (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIDEOANALYSIS))
#define GST_VIDEOANALYSIS_GET_CLASS(obj)  \
        (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_VIDEOANALYSIS,GstVideoAnalysisClass))

typedef struct _GstVideoAnalysis GstVideoAnalysis;
typedef struct _GstVideoAnalysisClass GstVideoAnalysisClass;

struct _GstVideoAnalysis
{
        GstGLBaseFilter    parent;

        /* <private> */
        gboolean           gl_settings_unchecked;
        /* GL stuff */
        GstGLShader *      shader;
        /* Textures */
        GstGLMemory *      tex;
        GstBuffer   *      prev_buffer;
        GstGLMemory *      prev_tex;
        /* VideoInfo */
        GstVideoInfo       in_info;
        GstVideoInfo       out_info;
        /* Interm values */
        float       values [PARAM_NUMBER];

        /* Frame-related data */
        guint       frame;
        guint       frame_limit;
        float       fps_period;
        guint       frames_in_sec;
        gfloat      cont_err_duration [PARAM_NUMBER];
        VideoParams params;
        Error       errors [PARAM_NUMBER];
        
        /* <public> */
        guint       period;
        gfloat      loss;
        guint       black_pixel_lb;
        guint       pixel_diff_lb;
        BOUNDARY    params_boundary [PARAM_NUMBER];
};

struct _GstVideoAnalysisClass
{
        GstGLBaseFilterClass parent_class;

        void (*data_signal) (GstVideoFilter *filter, GstBuffer* d);
};


G_END_DECLS

#endif /* _GSTVIDEOANALYSIS_ */
