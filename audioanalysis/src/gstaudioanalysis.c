/* gstaudioanalysis.c
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
 * SECTION:element-gstaudioanalysis
 *
 * The audioanalysis element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! audioanalysis ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/audio/gstaudiofilter.h>
#include "ebur128.h"
#include "gstaudioanalysis.h"

#define DIFF(x,y)((x > y)?(x-y):(y-x))

GST_DEBUG_CATEGORY_STATIC (gst_audioanalysis_debug_category);
#define GST_CAT_DEFAULT gst_audioanalysis_debug_category

/* prototypes */


static void
gst_audioanalysis_set_property (GObject * object,
				guint property_id,
				const GValue * value,
				GParamSpec * pspec);
static void
gst_audioanalysis_get_property (GObject * object,
				guint property_id,
				GValue * value,
				GParamSpec * pspec);
static void
gst_audioanalysis_dispose (GObject * object);

static void
gst_audioanalysis_finalize (GObject * object);

static gboolean
gst_audioanalysis_setup (GstAudioFilter * filter,
			 const GstAudioInfo * info);
static GstFlowReturn
gst_audioanalysis_transform_ip (GstBaseTransform * trans,
				GstBuffer * buf);
static inline void
gst_audioanalysis_eval_global (GstBaseTransform * trans,
			       guint ad_flag);
static gboolean
gst_filter_sink_ad_event (GstBaseTransform * parent,
			  GstEvent * event);

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
        PROP_PROGRAM,
        PROP_LOSS,
        PROP_ADV_DIFF,
        PROP_ADV_BUF,
        PROP_SILENCE_CONT,
        PROP_SILENCE_CONT_EN,
        PROP_SILENCE_PEAK,
        PROP_SILENCE_PEAK_EN,
        PROP_SILENCE_DURATION,
        PROP_LOUDNESS_CONT,
        PROP_LOUDNESS_CONT_EN,
        PROP_LOUDNESS_PEAK,
        PROP_LOUDNESS_PEAK_EN,
        PROP_LOUDNESS_DURATION,
        LAST_PROP
};

static guint      signals[LAST_SIGNAL]   = { 0 };
static GParamSpec *properties[LAST_PROP] = { NULL, };

/* pad templates */

static GstStaticPadTemplate gst_audioanalysis_src_template =
        GST_STATIC_PAD_TEMPLATE ("src",
                                 GST_PAD_SRC,
                                 GST_PAD_ALWAYS,
                                 GST_STATIC_CAPS ("audio/x-raw,format=S16LE,rate=[1,max],"
                                                  "channels=[1,max],layout=interleaved"));

static GstStaticPadTemplate gst_audioanalysis_sink_template =
        GST_STATIC_PAD_TEMPLATE ("sink",
                                 GST_PAD_SINK,
                                 GST_PAD_ALWAYS,
                                 GST_STATIC_CAPS ("audio/x-raw,format=S16LE,rate=[1,max],"
                                                  "channels=[1,max],layout=interleaved"));


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstAudioAnalysis,
			 gst_audioanalysis,
			 GST_TYPE_AUDIO_FILTER,
			 GST_DEBUG_CATEGORY_INIT (gst_audioanalysis_debug_category,
						  "audioanalysis", 0,
						  "debug category for audioanalysis element"));

static void
gst_audioanalysis_class_init (GstAudioAnalysisClass * klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
        GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
        GstAudioFilterClass *audio_filter_class = GST_AUDIO_FILTER_CLASS (klass);

        /* Setting up pads and setting metadata should be moved to
           base_class_init if you intend to subclass this class. */
        gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
                                            gst_static_pad_template_get (&gst_audioanalysis_src_template));
        gst_element_class_add_pad_template (GST_ELEMENT_CLASS(klass),
                                            gst_static_pad_template_get (&gst_audioanalysis_sink_template));

        gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
                                               "Gstreamer element for audio analysis",
                                               "Audio data analysis",
                                               "filter for audio analysis",
                                               "freyr <sky_rider_93@mail.ru>");

        gobject_class->set_property = gst_audioanalysis_set_property;
        gobject_class->get_property = gst_audioanalysis_get_property;
        gobject_class->dispose = gst_audioanalysis_dispose;
        gobject_class->finalize = gst_audioanalysis_finalize;
        audio_filter_class->setup = GST_DEBUG_FUNCPTR (gst_audioanalysis_setup);
        base_transform_class->transform_ip = GST_DEBUG_FUNCPTR (gst_audioanalysis_transform_ip);
        base_transform_class->sink_event = GST_DEBUG_FUNCPTR (gst_filter_sink_ad_event);
  

        signals[DATA_SIGNAL] =
                g_signal_new("data", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                             G_STRUCT_OFFSET(GstAudioAnalysisClass, data_signal), NULL, NULL,
                             g_cclosure_marshal_generic, G_TYPE_NONE,
                             4, G_TYPE_UINT64, GST_TYPE_BUFFER, G_TYPE_UINT64, GST_TYPE_BUFFER);

        properties [PROP_PROGRAM] =
                g_param_spec_int("program", "Program",
                                 "Program",
                                 0, G_MAXINT, 2010, G_PARAM_READWRITE);
        properties [PROP_LOSS] =
                g_param_spec_float("loss", "Loss",
                                   "Audio loss",
                                   0., G_MAXFLOAT, 1., G_PARAM_READWRITE);
        properties [PROP_ADV_DIFF] =
                g_param_spec_float("adv_diff", "Adv sound level diff",
                                   "Adv sound level diff",
                                   0., 1., 0.5, G_PARAM_READWRITE);
        properties [PROP_ADV_BUF] =
                g_param_spec_int("adv_buff", "Size of adv buffer",
                                 "Size of adv buffer",
                                 0, 1000, 100, G_PARAM_READWRITE);
        properties [PROP_SILENCE_CONT] =
                g_param_spec_float("silence_cont", "Silence cont err boundary",
                                   "Silence cont err meas",
                                   0., 1., 1., G_PARAM_READWRITE);
        properties [PROP_SILENCE_CONT_EN] =
                g_param_spec_boolean("silence_cont_en", "Silence cont err enabled",
                                     "Enable silence cont err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_SILENCE_PEAK] =
                g_param_spec_float("silence_peak", "Silence peak err boundary",
                                   "Silence peak err meas",
                                   0., 1., 1., G_PARAM_READWRITE);
        properties [PROP_SILENCE_PEAK_EN] =
                g_param_spec_boolean("silence_peak_en", "Silence peak err enabled",
                                     "Enable silence peak err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_SILENCE_DURATION] =
                g_param_spec_float("silence_duration", "Silence duration boundary",
                                   "Silence err duration",
                                   0., 1., 1., G_PARAM_READWRITE);
        properties [PROP_LOUDNESS_CONT] =
                g_param_spec_float("loudness_cont", "Loudness cont err boundary",
                                   "Loudness cont err meas",
                                   0., 1., 1., G_PARAM_READWRITE);
        properties [PROP_LOUDNESS_CONT_EN] =
                g_param_spec_boolean("loudness_cont_en", "Loudness cont err enabled",
                                     "Enable loudness cont err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_LOUDNESS_PEAK] =
                g_param_spec_float("loudness_peak", "Loudness peak err boundary",
                                   "Loudness peak err meas",
                                   0., 1., 1., G_PARAM_READWRITE);
        properties [PROP_LOUDNESS_PEAK_EN] =
                g_param_spec_boolean("loudness_peak_en", "Loudness peak err enabled",
                                     "Enable loudness peak err meas", FALSE, G_PARAM_READWRITE);
        properties [PROP_LOUDNESS_DURATION] =
                g_param_spec_float("loudness_duration", "Loudness duration boundary",
                                   "Loudness err duration",
                                   0., 1., 1., G_PARAM_READWRITE);
  
        g_object_class_install_properties(gobject_class, LAST_PROP, properties);
}

static void
gst_audioanalysis_init (GstAudioAnalysis *audioanalysis)
{
        audioanalysis->program = 2010;
        audioanalysis->loss = 1.;
        audioanalysis->adv_diff = 0.5;
        audioanalysis->adv_buf = 100;
        for (int i = 0; i < PARAM_NUMBER; i++) {
                audioanalysis->params_boundary[i].cont = 1.;
                audioanalysis->params_boundary[i].peak = 1.;
                audioanalysis->params_boundary[i].cont_en = FALSE;
                audioanalysis->params_boundary[i].peak_en = FALSE;
                audioanalysis->params_boundary[i].duration = 1.;
        }
        /* private */
        for (int i = 0; i < PARAM_NUMBER; i++) {
                audioanalysis->cont_err_duration[i] = 0.;
        }
        audioanalysis->state = NULL;
        audioanalysis->data = NULL;
        audioanalysis->time = 0;
        audioanalysis->glob_state = NULL;
        audioanalysis->glob_ad_flag = FALSE;
        audioanalysis->glob_start = 0;
}

void
gst_audioanalysis_set_property (GObject * object,
				guint property_id,
				const GValue * value,
				GParamSpec * pspec)
{
        GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (object);

        GST_DEBUG_OBJECT (audioanalysis, "set_property");

        switch (property_id) {
        case PROP_PROGRAM:
                audioanalysis->program = g_value_get_int(value);
                break;
        case PROP_LOSS:
                audioanalysis->loss = g_value_get_float(value);
                break;
        case PROP_ADV_DIFF:
                audioanalysis->adv_diff = g_value_get_float(value);
                break;
        case PROP_ADV_BUF:
                audioanalysis->adv_buf = g_value_get_int(value);
                break;
        case PROP_SILENCE_CONT:
                audioanalysis->params_boundary[SILENCE].cont = g_value_get_float(value);
                break;
        case PROP_SILENCE_CONT_EN:
                audioanalysis->params_boundary[SILENCE].cont_en = g_value_get_boolean(value);
                break;
        case PROP_SILENCE_PEAK:
                audioanalysis->params_boundary[SILENCE].peak = g_value_get_float(value);
                break;
        case PROP_SILENCE_PEAK_EN:
                audioanalysis->params_boundary[SILENCE].peak_en = g_value_get_boolean(value);
                break;
        case PROP_SILENCE_DURATION:
                audioanalysis->params_boundary[SILENCE].duration = g_value_get_float(value);
                break;
        case PROP_LOUDNESS_CONT:
                audioanalysis->params_boundary[LOUDNESS].cont = g_value_get_float(value);
                break;
        case PROP_LOUDNESS_CONT_EN:
                audioanalysis->params_boundary[LOUDNESS].cont_en = g_value_get_boolean(value);
                break;
        case PROP_LOUDNESS_PEAK:
                audioanalysis->params_boundary[LOUDNESS].peak = g_value_get_float(value);
                break;
        case PROP_LOUDNESS_PEAK_EN:
                audioanalysis->params_boundary[LOUDNESS].peak_en = g_value_get_boolean(value);
                break;
        case PROP_LOUDNESS_DURATION:
                audioanalysis->params_boundary[LOUDNESS].duration = g_value_get_float(value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

void
gst_audioanalysis_get_property (GObject * object,
				guint property_id,
				GValue * value,
				GParamSpec * pspec)
{
        GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (object);

        GST_DEBUG_OBJECT (audioanalysis, "get_property");

        switch (property_id) {
        case PROP_PROGRAM: 
                g_value_set_float(value, audioanalysis->program);
                break;
        case PROP_LOSS: 
                g_value_set_float(value, audioanalysis->loss);
                break;
        case PROP_ADV_DIFF: 
                g_value_set_float(value, audioanalysis->adv_diff);
                break;
        case PROP_ADV_BUF: 
                g_value_set_float(value, audioanalysis->adv_buf);
                break;
        case PROP_SILENCE_CONT:
                g_value_set_float(value, audioanalysis->params_boundary[SILENCE].cont);
                break;
        case PROP_SILENCE_CONT_EN:
                g_value_set_boolean(value, audioanalysis->params_boundary[SILENCE].cont_en);
                break;
        case PROP_SILENCE_PEAK:
                g_value_set_float(value, audioanalysis->params_boundary[SILENCE].peak);
                break;
        case PROP_SILENCE_PEAK_EN:
                g_value_set_boolean(value, audioanalysis->params_boundary[SILENCE].peak_en);
                break;
        case PROP_SILENCE_DURATION:
                g_value_set_float(value, audioanalysis->params_boundary[SILENCE].duration);
                break;
        case PROP_LOUDNESS_CONT:
                g_value_set_float(value, audioanalysis->params_boundary[LOUDNESS].cont);
                break;
        case PROP_LOUDNESS_CONT_EN:
                g_value_set_boolean(value, audioanalysis->params_boundary[LOUDNESS].cont_en);
                break;
        case PROP_LOUDNESS_PEAK:
                g_value_set_float(value, audioanalysis->params_boundary[LOUDNESS].peak);
                break;
        case PROP_LOUDNESS_PEAK_EN:
                g_value_set_boolean(value, audioanalysis->params_boundary[LOUDNESS].peak_en);
                break;
        case PROP_LOUDNESS_DURATION:
                g_value_set_float(value, audioanalysis->params_boundary[LOUDNESS].duration);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

void
gst_audioanalysis_dispose (GObject * object)
{
        GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (object);
  
        GST_DEBUG_OBJECT (audioanalysis, "dispose");

        /* clean up as possible.  may be called multiple times */

        G_OBJECT_CLASS (gst_audioanalysis_parent_class)->dispose (object);
}

void
gst_audioanalysis_finalize (GObject * object)
{
        GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (object);

        GST_DEBUG_OBJECT (audioanalysis, "finalize");

        if (audioanalysis->state != NULL)
                ebur128_destroy(&audioanalysis->state);
        if (audioanalysis->glob_state != NULL)
                ebur128_destroy(&audioanalysis->glob_state);

        if (audioanalysis->data != NULL)
                audio_data_delete(audioanalysis->data);
        if (audioanalysis->errors != NULL)
                errors_delete(audioanalysis->errors);

        //gst_object_unref(audioanalysis->clock);

        G_OBJECT_CLASS (gst_audioanalysis_parent_class)->finalize (object);
}

static gboolean
gst_audioanalysis_setup (GstAudioFilter * filter,
			 const GstAudioInfo * info)
{
        GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (filter);

        GST_DEBUG_OBJECT (audioanalysis, "setup");

        if (audioanalysis->state != NULL)
                ebur128_destroy(&audioanalysis->state);
        if (audioanalysis->glob_state != NULL)
                ebur128_destroy(&audioanalysis->glob_state);
  
        audioanalysis->state = ebur128_init(info->channels,
                                            (unsigned long)info->rate,
                                            EBUR128_MODE_S | EBUR128_MODE_M);
        audioanalysis->glob_state = ebur128_init(info->channels,
                                                 (unsigned long)info->rate,
                                                 EBUR128_MODE_I);

        if (audioanalysis->data != NULL)
                audio_data_delete(audioanalysis->data);
        if (audioanalysis->errors != NULL)
                errors_delete(audioanalysis->errors);
        
        audioanalysis->data = audio_data_new(EVAL_PERIOD);
        audioanalysis->errors = errors_new(EVAL_PERIOD);

        audioanalysis->time = gst_clock_get_time(GST_ELEMENT(audioanalysis)->clock);
  
        return TRUE;
}

static inline void
gst_audioanalysis_eval_global (GstBaseTransform * trans,
			       guint ad_flag)
{
        /*
        GstAudioAnalysis *audioanalysis;
        double           result, diff_time;
        time_t           now;
        gchar            string[50];

        audioanalysis = GST_AUDIOANALYSIS (trans);
        now = time(NULL);

        // if measurements have already begun
        if (audioanalysis->glob_ad_flag) {

                ebur128_loudness_global(audioanalysis->glob_state, &result);
                ebur128_clear_block_list(audioanalysis->glob_state);

                diff_time = difftime(audioanalysis->glob_start, now);

                g_snprintf(string, 39, "c%d:%d:%d:*:%.2f:%.2f:%d",
                           audioanalysis->stream_id,
                           audioanalysis->program,
                           audioanalysis->pid,
                           result, diff_time, ad_flag);

                gst_audioanalysis_send_string(string, audioanalysis);
        } else {
                audioanalysis->glob_ad_flag = TRUE;
        }

        audioanalysis->glob_start = now;
        */
}

/* transform */
static GstFlowReturn
gst_audioanalysis_transform_ip (GstBaseTransform * trans,
				GstBuffer * buf)
{
        GstAudioAnalysis *audioanalysis = GST_AUDIOANALYSIS (trans);
        GstMapInfo map;
        guint num_frames;
        //time_t now;
        AudioParams params;
        GstClockTime current_time = gst_clock_get_time(GST_ELEMENT(audioanalysis)->clock);

        GST_DEBUG_OBJECT (audioanalysis, "transform_ip");
  
        gst_buffer_map(buf, &map, GST_MAP_READ);
        num_frames = map.size / (GST_AUDIO_FILTER_BPS(audioanalysis) * GST_AUDIO_FILTER_CHANNELS(audioanalysis));

        ebur128_add_frames_short(audioanalysis->state, (short*)map.data, num_frames);

        /* add frames to an ad state */
        /* if (audioanalysis->glob_ad_flag) {

                now = time(NULL);

                ebur128_add_frames_short(audioanalysis->glob_state, (short*)map.data, num_frames);

                // interval exceeded specified timeout
                if (DIFF(now, audioanalysis->glob_start) >= audioanalysis->ad_timeout) {
                        ebur128_clear_block_list(audioanalysis->glob_state);
                        audioanalysis->glob_ad_flag = FALSE;
                }
                }*/
    
        /* send data for the momentary and short term states */
        if (audio_data_is_full(audioanalysis->data)
            || errors_is_full(audioanalysis->errors)) {
                
                gsize ds, es;
                gpointer d = audio_data_dump(audioanalysis->data, &ds);
                gpointer e = errors_dump(audioanalysis->errors, &es);

                audio_data_reset(audioanalysis->data);
                errors_reset(audioanalysis->errors);

                GstBuffer* db = gst_buffer_new_wrapped (d, sizeof(AudioParams) * ds);
                GstBuffer* eb = gst_buffer_new_wrapped (e, sizeof(ErrFlags) * es * PARAM_NUMBER);
                
                g_signal_emit(audioanalysis, signals[DATA_SIGNAL], 0, ds, db, es, eb);
        }

        /* eval loudness for the 100ms interval */
        if (DIFF(current_time, audioanalysis->time) >= OBSERVATION_TIME) {
                gint64 tm = g_get_real_time ();
                ErrFlags eflags[PARAM_NUMBER];
    
                ebur128_loudness_momentary(audioanalysis->state, &(params.moment));
                ebur128_loudness_shortterm(audioanalysis->state, &(params.shortt));
                params.time = tm;

                /* errors */
                for (int p = 0; p < PARAM_NUMBER; p++) {
                        float par = params.moment;
                        err_flags_cmp(&(eflags[p]),
                                      &(audioanalysis->params_boundary[p]),
                                      tm, param_upper_boundary(p),
                                      &(audioanalysis->cont_err_duration[p]),
                                      0.1 /* 100ms */,
                                      par);
                }
                audio_data_append(audioanalysis->data, &params);
                errors_append(audioanalysis->errors, eflags);
                
                audioanalysis->time = current_time;
        }
  
        gst_buffer_unmap(buf, &map);

        return GST_FLOW_OK;
}

static gboolean
gst_filter_sink_ad_event (GstBaseTransform * base,
			  GstEvent * event)
{
        GstAudioAnalysis       *filter;
        const GstStructure     *st; 
        guint                  pid;
        guint                  ad;
  
        filter = GST_AUDIOANALYSIS(base);
  
        if (GST_EVENT_TYPE (event) == GST_EVENT_CUSTOM_DOWNSTREAM) { 

                st = gst_event_get_structure(event);

                if (gst_structure_has_name(st, "ad")) {
      
                        pid = g_value_get_uint(gst_structure_get_value(st, "pid"));
                        ad  = g_value_get_uint(gst_structure_get_value(st, "isad"));
      
                        if ((guint)filter->program == pid) {

                                gst_audioanalysis_eval_global(base, ad);
	
                                gst_event_unref(event);
                                event = NULL;
                        }
                }
        }
        /* pass event on */
        if (event)
                return GST_BASE_TRANSFORM_CLASS
                        (gst_audioanalysis_parent_class)->sink_event (base, event);
        else 
                return TRUE;  
}

static gboolean
plugin_init (GstPlugin * plugin)
{

        /* FIXME Remember to set the rank if it's an element that is meant
           to be autoplugged by decodebin. */
        return gst_element_register (plugin,
                                     "audioanalysis",
                                     GST_RANK_NONE,
                                     GST_TYPE_AUDIOANALYSIS);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.1.9"
#endif
#ifndef PACKAGE
#define PACKAGE "audioanalysis"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "audioanalysis_package"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN URL
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
		   GST_VERSION_MINOR,
		   audioanalysis,
		   "Package for audio data analysis",
		   plugin_init, VERSION, LIC, PACKAGE_NAME, GST_PACKAGE_ORIGIN)

