/* gstaudioanalysis.h
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

#ifndef _GST_AUDIOANALYSIS_H_
#define _GST_AUDIOANALYSIS_H_

#include <time.h>
#include <gst/audio/gstaudiofilter.h>
#include "ebur128.h"

#include "audiodata.h"
#include "error.h"

#define OBSERVATION_TIME 100000000
#define EVAL_PERIOD 10

G_BEGIN_DECLS

#define GST_TYPE_AUDIOANALYSIS                  \
        (gst_audioanalysis_get_type())
#define GST_AUDIOANALYSIS(obj)                                          \
        (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_AUDIOANALYSIS,GstAudioAnalysis))
#define GST_AUDIOANALYSIS_CLASS(klass)                                  \
        (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_AUDIOANALYSIS,GstAudioAnalysisClass))
#define GST_IS_AUDIOANALYSIS(obj)                                       \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_AUDIOANALYSIS))
#define GST_IS_AUDIOANALYSIS_CLASS(obj)                                 \
        (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_AUDIOANALYSIS))

typedef struct _GstAudioAnalysis GstAudioAnalysis;
typedef struct _GstAudioAnalysisClass GstAudioAnalysisClass;

struct _GstAudioAnalysis
{
        GstAudioFilter base_audioanalysis;
        /* Public */
        /* TODO add later: period */
        int      program;
        float    loss;
        gfloat   adv_diff;
        gint     adv_buf;
        BOUNDARY params_boundary [PARAM_NUMBER];
        /* Private */
        gfloat cont_err_duration [PARAM_NUMBER];
        ebur128_state *state;
        AudioData     *data;
        Errors        *errors;
        /* Global loudless */
        ebur128_state *glob_state;
        gboolean      glob_ad_flag;
        time_t        glob_start;
        //  GstClock* clock;
        GstClockTime time;
};

struct _GstAudioAnalysisClass
{
        GstAudioFilterClass base_audioanalysis_class;

        void (*data_signal) (GstAudioFilter *filter, guint64 ds, GstBuffer* d, guint64 es, GstBuffer* e);
};

GType gst_audioanalysis_get_type (void);

G_END_DECLS

#endif
