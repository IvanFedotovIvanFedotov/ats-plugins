/* gstvideoanalysis.c
 *
 * Copyright (C) 2016 freyr <sky_rider_93@mail.ru> 
 *
 * This file is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 3 of the 
 * License, or (at your option) any later version. 
 *
 * This file is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef LGPL_LIC
#define LIC "Proprietary"
#define URL "http://www.niitv.ru/"
#else
#define LIC "LGPL"
#define URL "https://github.com/Freyr666/ats-analyzer/"
#endif

/**
 * SECTION:element-gstvideoanalysis
 *
 * The videoanalysis element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! videoanalysis ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <string.h>
#include <malloc.h>
#include <glib.h>

#include "gstvideoanalysis.h"

#include "analysis.h"

GST_DEBUG_CATEGORY_STATIC (gst_videoanalysis_debug_category);
#define GST_CAT_DEFAULT gst_videoanalysis_debug_category

/* prototypes */

static void
gst_videoanalysis_set_property (GObject * object,
				guint property_id,
				const GValue * value,
				GParamSpec * pspec);
static void
gst_videoanalysis_get_property (GObject * object,
				guint property_id,
				GValue * value,
				GParamSpec * pspec);
static void
gst_videoanalysis_dispose      (GObject * object);
static void
gst_videoanalysis_finalize     (GObject * object);

static gboolean
gst_videoanalysis_start        (GstBaseTransform * trans);
static gboolean
gst_videoanalysis_stop         (GstBaseTransform * trans);
static gboolean
gst_videoanalysis_set_info     (GstVideoFilter * filter,
				GstCaps * incaps,
				GstVideoInfo * in_info,
				GstCaps * outcaps,
				GstVideoInfo * out_info);

static GstFlowReturn
gst_videoanalysis_transform_frame_ip (GstVideoFilter * filter,
				      GstVideoFrame * frame);

/* signals */
enum
{
        DATA_SIGNAL,
        LAST_SIGNAL
};

/* args */
enum
{
        PROP_0,
        PROP_PERIOD,
        PROP_LOSS,
        PROP_BLACK_PIXEL_LB,
        PROP_PIXEL_DIFF_LB,
        PROP_BLACK_CONT,
        PROP_BLACK_CONT_EN,
        PROP_BLACK_PEAK,
        PROP_BLACK_PEAK_EN,
        PROP_BLACK_DURATION,
        PROP_LUMA_CONT,
        PROP_LUMA_CONT_EN,
        PROP_LUMA_PEAK,
        PROP_LUMA_PEAK_EN,
        PROP_LUMA_DURATION,
        PROP_FREEZE_CONT,
        PROP_FREEZE_CONT_EN,
        PROP_FREEZE_PEAK,
        PROP_FREEZE_PEAK_EN,
        PROP_FREEZE_DURATION,
        PROP_DIFF_CONT,
        PROP_DIFF_CONT_EN,
        PROP_DIFF_PEAK,
        PROP_DIFF_PEAK_EN,
        PROP_DIFF_DURATION,
        PROP_BLOCKY_CONT,
        PROP_BLOCKY_CONT_EN,
        PROP_BLOCKY_PEAK,
        PROP_BLOCKY_PEAK_EN,
        PROP_BLOCKY_DURATION,
        PROP_MARK_BLOCKS,
        LAST_PROP
};

static guint      signals[LAST_SIGNAL]   = { 0 };
static GParamSpec *properties[LAST_PROP] = { NULL, };

/* pad templates */

#define VIDEO_SRC_CAPS                                          \
        GST_VIDEO_CAPS_MAKE("{ I420, NV12, NV21, YV12, IYUV }")

#define VIDEO_SINK_CAPS                                         \
        GST_VIDEO_CAPS_MAKE("{ I420, NV12, NV21, YV12, IYUV }")

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
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
        GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
        GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS (klass);

        /* Setting up pads and setting metadata should be moved to
           base_class_init if you intend to subclass this class. */
        gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
                                            gst_pad_template_new ("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  gst_caps_from_string (VIDEO_SRC_CAPS)));
        gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
                                            gst_pad_template_new ("sink",
                                                                  GST_PAD_SINK,
                                                                  GST_PAD_ALWAYS,
                                                                  gst_caps_from_string (VIDEO_SINK_CAPS)));

        gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
                                               "Gstreamer element for video analysis",
                                               "Video data analysis",
                                               "filter for video analysis",
                                               "freyr <sky_rider_93@mail.ru>");

        gobject_class->set_property = gst_videoanalysis_set_property;
        gobject_class->get_property = gst_videoanalysis_get_property;
        gobject_class->dispose = gst_videoanalysis_dispose;
        gobject_class->finalize = gst_videoanalysis_finalize;
        base_transform_class->start = GST_DEBUG_FUNCPTR (gst_videoanalysis_start);
        base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_videoanalysis_stop);
        video_filter_class->set_info = GST_DEBUG_FUNCPTR (gst_videoanalysis_set_info);
        video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_videoanalysis_transform_frame_ip);

        signals[DATA_SIGNAL] =
                g_signal_new("data", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                             G_STRUCT_OFFSET(GstVideoAnalysisClass, data_signal), NULL, NULL,
                             g_cclosure_marshal_generic, G_TYPE_NONE,
                             4, G_TYPE_UINT64, GST_TYPE_BUFFER, G_TYPE_UINT64, GST_TYPE_BUFFER);
        
        properties [PROP_PERIOD] =
                g_param_spec_float("period", "Period",
                                   "Period of time between info masseges",
                                   0.1, 5., 0.5, G_PARAM_READWRITE);
        properties [PROP_LOSS] =
                g_param_spec_float("loss", "Loss",
                                   "Video loss",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_BLACK_PIXEL_LB] =
                g_param_spec_uint("black_pixel_lb", "Black pixel lb",
                                  "Black pixel value lower boundary",
                                  0, 256, 16, G_PARAM_READWRITE);
        properties [PROP_PIXEL_DIFF_LB] =
                g_param_spec_uint("pixel_diff_lb", "Freeze pixel lb",
                                  "Freeze pixel value lower boundary",
                                  0, 256, 0, G_PARAM_READWRITE);
        properties [PROP_BLACK_CONT] =
                g_param_spec_float("black_cont", "Black cont err boundary",
                                   "Black cont err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_BLACK_CONT_EN] =
                g_param_spec_boolean("black_cont_en", "Black cont err enabled",
                                     "Enable black cont err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_BLACK_PEAK] =
                g_param_spec_float("black_peak", "Black peak err boundary",
                                   "Black peak err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_BLACK_PEAK_EN] =
                g_param_spec_boolean("black_peak_en", "Black peak err enabled",
                                     "Enable black peak err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_BLACK_DURATION] =
                g_param_spec_float("black_duration", "Black duration boundary",
                                   "Black err duration",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_LUMA_CONT] =
                g_param_spec_float("luma_cont", "Luma cont err boundary",
                                   "Luma cont err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_LUMA_CONT_EN] =
                g_param_spec_boolean("luma_cont_en", "Luma cont err enabled",
                                     "Enable luma cont err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_LUMA_PEAK] =
                g_param_spec_float("luma_peak", "Luma peak err boundary",
                                   "Luma peak err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_LUMA_PEAK_EN] =
                g_param_spec_boolean("luma_peak_en", "Luma peak err enabled",
                                     "Enable luma peak err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_LUMA_DURATION] =
                g_param_spec_float("luma_duration", "Luma duration boundary",
                                   "Luma err duration",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_FREEZE_CONT] =
                g_param_spec_float("freeze_cont", "Freeze cont err boundary",
                                   "Freeze cont err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_FREEZE_CONT_EN] =
                g_param_spec_boolean("freeze_cont_en", "Freeze cont err enabled",
                                     "Enable freeze cont err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_FREEZE_PEAK] =
                g_param_spec_float("freeze_peak", "Freeze peak err boundary",
                                   "Freeze peak err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_FREEZE_PEAK_EN] =
                g_param_spec_boolean("freeze_peak_en", "Freeze peak err enabled",
                                     "Enable freeze peak err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_FREEZE_DURATION] =
                g_param_spec_float("freeze_duration", "Freeze duration boundary",
                                   "Freeze err duration",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_DIFF_CONT] =
                g_param_spec_float("freeze_diff_cont", "Diff cont err boundary",
                                   "Diff cont err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_DIFF_CONT_EN] =
                g_param_spec_boolean("diff_cont_en", "Diff cont err enabled",
                                     "Enable diff cont err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_DIFF_PEAK] =
                g_param_spec_float("diff_peak", "Diff peak err boundary",
                                   "Diff peak err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_DIFF_PEAK_EN] =
                g_param_spec_boolean("diff_peak_en", "Diff peak err enabled",
                                     "Enable diff peak err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_DIFF_DURATION] =
                g_param_spec_float("diff_duration", "Diff duration boundary",
                                   "Diff err duration",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_BLOCKY_CONT] =
                g_param_spec_float("blocky_cont", "Blocky cont err boundary",
                                   "Blocky cont err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_BLOCKY_CONT_EN] =
                g_param_spec_boolean("blocky_cont_en", "Blocky cont err enabled",
                                     "Enable blocky cont err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_BLOCKY_PEAK] =
                g_param_spec_float("blocky_peak", "Blocky peak err boundary",
                                   "Blocky peak err meas",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_BLOCKY_PEAK_EN] =
                g_param_spec_boolean("blocky_peak_en", "Blocky peak err enabled",
                                     "Enable blocky peak err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_BLOCKY_DURATION] =
                g_param_spec_float("blocky_duration", "Blocky duration boundary",
                                   "Blocky err duration",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_MARK_BLOCKS] =
                g_param_spec_uint("mark_blocks", "Mark_blocks",
                                  "Mark borders of visible blocks",
                                  0, 256, 0, G_PARAM_READWRITE);

        g_object_class_install_properties(gobject_class, LAST_PROP, properties);
}

static void
gst_videoanalysis_init (GstVideoAnalysis *videoanalysis)
{
        videoanalysis->fps_period = 1.;
        videoanalysis->loss    = 1.;
        videoanalysis->black_pixel_lb = 16;
        videoanalysis->pixel_diff_lb = 0;
        for (guint i = 0; i < PARAM_NUMBER; i++) {
                        videoanalysis->params_boundary[i].cont = 1.;
                        videoanalysis->params_boundary[i].peak = 1.;
                        videoanalysis->params_boundary[i].cont_en = FALSE;
                        videoanalysis->params_boundary[i].peak_en = FALSE;
                        videoanalysis->params_boundary[i].duration = 1.;
        }
        videoanalysis->mark_blocks = 0;
        videoanalysis->period = 0.5;
        /* private */
        for (guint i = 0; i < PARAM_NUMBER; i++) {
                videoanalysis->cont_err_duration[i] = 0.;
        }
        videoanalysis->past_buffer = (guint8*)malloc(4096*4096);
        videoanalysis->blocks = (BLOCK*)malloc(512*512);
}

void
gst_videoanalysis_set_property (GObject * object,
				guint property_id,
				const GValue * value,
				GParamSpec * pspec)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (object);

        GST_DEBUG_OBJECT (videoanalysis, "set_property");

        switch (property_id) {
        case PROP_PERIOD:
                videoanalysis->period = g_value_get_float(value);
                break;
        case PROP_LOSS:
                videoanalysis->loss = g_value_get_float(value);
                break;
        case PROP_BLACK_PIXEL_LB:
                videoanalysis->black_pixel_lb = g_value_get_uint(value);
                break;
        case PROP_PIXEL_DIFF_LB:
                videoanalysis->pixel_diff_lb = g_value_get_uint(value);
                break;
        case PROP_BLACK_CONT:
                videoanalysis->params_boundary[BLACK].cont = g_value_get_float(value);
                break;
        case PROP_BLACK_CONT_EN:
                videoanalysis->params_boundary[BLACK].cont_en = g_value_get_boolean(value);
                break;
        case PROP_BLACK_PEAK:
                videoanalysis->params_boundary[BLACK].peak = g_value_get_float(value);
                break;
        case PROP_BLACK_PEAK_EN:
                videoanalysis->params_boundary[BLACK].peak_en = g_value_get_boolean(value);
                break;
        case PROP_BLACK_DURATION:
                videoanalysis->params_boundary[BLACK].duration = g_value_get_float(value);
                break;
        case PROP_LUMA_CONT:
                videoanalysis->params_boundary[LUMA].cont = g_value_get_float(value);
                break;
        case PROP_LUMA_CONT_EN:
                videoanalysis->params_boundary[LUMA].cont_en = g_value_get_boolean(value);
                break;
        case PROP_LUMA_PEAK:
                videoanalysis->params_boundary[LUMA].peak = g_value_get_float(value);
                break;
        case PROP_LUMA_PEAK_EN:
                videoanalysis->params_boundary[LUMA].peak_en = g_value_get_boolean(value);
                break;
        case PROP_LUMA_DURATION:
                videoanalysis->params_boundary[LUMA].duration = g_value_get_float(value);
                break;
        case PROP_FREEZE_CONT:
                videoanalysis->params_boundary[FREEZE].cont = g_value_get_float(value);
                break;
        case PROP_FREEZE_CONT_EN:
                videoanalysis->params_boundary[FREEZE].cont_en = g_value_get_boolean(value);
                break;
        case PROP_FREEZE_PEAK:
                videoanalysis->params_boundary[FREEZE].peak = g_value_get_float(value);
                break;
        case PROP_FREEZE_PEAK_EN:
                videoanalysis->params_boundary[FREEZE].peak_en = g_value_get_boolean(value);
                break;
        case PROP_FREEZE_DURATION:
                videoanalysis->params_boundary[FREEZE].duration = g_value_get_float(value);
                break;
        case PROP_DIFF_CONT:
                videoanalysis->params_boundary[DIFF].cont = g_value_get_float(value);
                break;
        case PROP_DIFF_CONT_EN:
                videoanalysis->params_boundary[DIFF].cont_en = g_value_get_boolean(value);
                break;
        case PROP_DIFF_PEAK:
                videoanalysis->params_boundary[DIFF].peak = g_value_get_float(value);
                break;
        case PROP_DIFF_PEAK_EN:
                videoanalysis->params_boundary[DIFF].peak_en = g_value_get_boolean(value);
                break;
        case PROP_DIFF_DURATION:
                videoanalysis->params_boundary[DIFF].duration = g_value_get_float(value);
                break;
        case PROP_BLOCKY_CONT:
                videoanalysis->params_boundary[BLOCKY].cont = g_value_get_float(value);
                break;
        case PROP_BLOCKY_CONT_EN:
                videoanalysis->params_boundary[BLOCKY].cont_en = g_value_get_boolean(value);
                break;
        case PROP_BLOCKY_PEAK:
                videoanalysis->params_boundary[BLOCKY].peak = g_value_get_float(value);
                break;
        case PROP_BLOCKY_PEAK_EN:
                videoanalysis->params_boundary[BLOCKY].peak_en = g_value_get_boolean(value);
                break;
        case PROP_BLOCKY_DURATION:
                videoanalysis->params_boundary[BLOCKY].duration = g_value_get_float(value);
                break;
        case PROP_MARK_BLOCKS:
                videoanalysis->mark_blocks = g_value_get_uint(value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

void
gst_videoanalysis_get_property (GObject * object,
				guint property_id,
				GValue * value,
				GParamSpec * pspec)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (object);

        GST_DEBUG_OBJECT (videoanalysis, "get_property");

        switch (property_id) {
        case PROP_PERIOD: 
                g_value_set_float(value, videoanalysis->period);
                break;
        case PROP_LOSS: 
                g_value_set_float(value, videoanalysis->loss);
                break;
        case PROP_BLACK_PIXEL_LB:
                g_value_set_uint(value, videoanalysis->black_pixel_lb);
                break;
        case PROP_PIXEL_DIFF_LB:
                g_value_set_uint(value, videoanalysis->pixel_diff_lb);
                break;
        case PROP_BLACK_CONT:
                g_value_set_float(value, videoanalysis->params_boundary[BLACK].cont);
                break;
        case PROP_BLACK_CONT_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[BLACK].cont_en);
                break;
        case PROP_BLACK_PEAK:
                g_value_set_float(value, videoanalysis->params_boundary[BLACK].peak);
                break;
        case PROP_BLACK_PEAK_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[BLACK].peak_en);
                break;
        case PROP_BLACK_DURATION:
                g_value_set_float(value, videoanalysis->params_boundary[BLACK].duration);
                break;
        case PROP_LUMA_CONT:
                g_value_set_float(value, videoanalysis->params_boundary[LUMA].cont);
                break;
        case PROP_LUMA_CONT_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[LUMA].cont_en);
                break;
        case PROP_LUMA_PEAK:
                g_value_set_float(value, videoanalysis->params_boundary[LUMA].peak);
                break;
        case PROP_LUMA_PEAK_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[LUMA].peak_en);
                break;
        case PROP_LUMA_DURATION:
                g_value_set_float(value, videoanalysis->params_boundary[LUMA].duration);
                break;
        case PROP_FREEZE_CONT:
                g_value_set_float(value, videoanalysis->params_boundary[FREEZE].cont);
                break;
        case PROP_FREEZE_CONT_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[FREEZE].cont_en);
                break;
        case PROP_FREEZE_PEAK:
                g_value_set_float(value, videoanalysis->params_boundary[FREEZE].peak);
                break;
        case PROP_FREEZE_PEAK_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[FREEZE].peak_en);
                break;
        case PROP_FREEZE_DURATION:
                g_value_set_float(value, videoanalysis->params_boundary[FREEZE].duration);
                break;
        case PROP_DIFF_CONT:
                g_value_set_float(value, videoanalysis->params_boundary[DIFF].cont);
                break;
        case PROP_DIFF_CONT_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[DIFF].cont_en);
                break;
        case PROP_DIFF_PEAK:
                g_value_set_float(value, videoanalysis->params_boundary[DIFF].peak);
                break;
        case PROP_DIFF_PEAK_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[DIFF].peak_en);
                break;
        case PROP_DIFF_DURATION:
                g_value_set_float(value, videoanalysis->params_boundary[DIFF].duration);
                break;
        case PROP_BLOCKY_CONT:
                g_value_set_float(value, videoanalysis->params_boundary[BLOCKY].cont);
                break;
        case PROP_BLOCKY_CONT_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[BLOCKY].cont_en);
                break;
        case PROP_BLOCKY_PEAK:
                g_value_set_float(value, videoanalysis->params_boundary[BLOCKY].peak);
                break;
        case PROP_BLOCKY_PEAK_EN:
                g_value_set_boolean(value, videoanalysis->params_boundary[BLOCKY].peak_en);
                break;
        case PROP_BLOCKY_DURATION:
                g_value_set_float(value, videoanalysis->params_boundary[BLOCKY].duration);
                break;
        case PROP_MARK_BLOCKS: 
                g_value_set_uint(value, videoanalysis->mark_blocks);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

void
gst_videoanalysis_dispose (GObject * object)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (object);

        GST_DEBUG_OBJECT (videoanalysis, "dispose");

        G_OBJECT_CLASS (gst_videoanalysis_parent_class)->dispose (object);
}

void
gst_videoanalysis_finalize (GObject * object)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (object);

        GST_DEBUG_OBJECT (videoanalysis, "finalize");

        free(videoanalysis->past_buffer);
        free(videoanalysis->blocks);
  
        G_OBJECT_CLASS (gst_videoanalysis_parent_class)->finalize (object);
}

static gboolean
gst_videoanalysis_start (GstBaseTransform * trans)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);

        GST_DEBUG_OBJECT (videoanalysis, "start");
 
        return TRUE;
}

static gboolean
gst_videoanalysis_stop (GstBaseTransform * trans)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (trans);

        GST_DEBUG_OBJECT (videoanalysis, "stop");

        if(videoanalysis->data != NULL)
                video_data_delete(videoanalysis->data);
        if(videoanalysis->errors != NULL)
                errors_delete(videoanalysis->errors);
        return TRUE;
}


static gboolean
gst_videoanalysis_set_info (GstVideoFilter * filter,
			    GstCaps * incaps,
			    GstVideoInfo * in_info,
			    GstCaps * outcaps,
			    GstVideoInfo * out_info)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (filter);

        GST_DEBUG_OBJECT (videoanalysis, "set_info");

        videoanalysis->fps_period = (float) in_info->fps_d / (float) in_info->fps_n;
        int period = (int)(videoanalysis->period / videoanalysis->fps_period);

        if (videoanalysis->data != NULL)
                video_data_delete(videoanalysis->data);
        if(videoanalysis->errors != NULL)
                errors_delete(videoanalysis->errors);
        
        videoanalysis->data   = video_data_new(period);
        videoanalysis->errors = errors_new(period);
        
        return TRUE;
}

/* transform */
static GstFlowReturn
gst_videoanalysis_transform_frame_ip (GstVideoFilter * filter,
				      GstVideoFrame * frame)
{
        GstVideoAnalysis *videoanalysis = GST_VIDEOANALYSIS (filter);
        VideoParams params;
        ErrFlags eflags[PARAM_NUMBER];
  
        GST_DEBUG_OBJECT (videoanalysis, "transform_frame_ip");

        gint64 tm = g_get_real_time ();
        
        if (video_data_is_full(videoanalysis->data)
            || errors_is_full(videoanalysis->errors) ){

                gsize ds, es;
                gpointer d = video_data_dump(videoanalysis->data, &ds);
                gpointer e = errors_dump(videoanalysis->errors, &es);
                        
                video_data_reset(videoanalysis->data);
                errors_reset(videoanalysis->errors);
                
                GstBuffer* db = gst_buffer_new_wrapped (d, sizeof(d));
                GstBuffer* eb = gst_buffer_new_wrapped (e, sizeof(e));

                g_signal_emit(videoanalysis, signals[DATA_SIGNAL], 0, ds, db, es, eb);
        }
        /* params */
        analyse_buffer(frame->data[0],
                       videoanalysis->past_buffer,
                       frame->info.stride[0],
                       frame->info.width,
                       frame->info.height,
                       videoanalysis->black_pixel_lb,
                       videoanalysis->pixel_diff_lb,
                       videoanalysis->mark_blocks,
                       videoanalysis->blocks,
                       &params);
        params.time = tm;
        /* errors */
        for (int p = 0; p < PARAM_NUMBER; p++) {
                float par = param_of_video_params(&params, p);
                err_flags_cmp(&(eflags[p]),
                              &(videoanalysis->params_boundary[p]),
                              tm, TRUE,
                              &(videoanalysis->cont_err_duration[p]),
                              videoanalysis->fps_period,
                              par);
        }
        /* append params and errors */
        video_data_append(videoanalysis->data, &params);        
        errors_append(videoanalysis->errors, eflags);        

        return GST_FLOW_OK;
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

