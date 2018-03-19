/* audiodata.h
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
#ifndef AUDIODATA_H
#define AUDIODATA_H

#include <glib.h>

#define DATA_MARKER 0x8BA820F0

typedef struct __AudioParams AudioParams;
typedef struct __AudioData AudioData;

struct __AudioParams {
        double shortt;
        double moment;
        gint64 time;
};

struct __AudioData {
        guint current;
        guint period;
        AudioParams* data;
};

AudioData* audio_data_new(guint fr);
#define audio_data_reset(dt)(dt->current = 0)
void audio_data_delete(AudioData* dt);
int  audio_data_append(AudioData* dt, AudioParams* par);
gboolean audio_data_is_full(AudioData* dt);
gpointer audio_data_dump(AudioData* dt, gsize* sz);
/* a(stream):(prog):(pid):*:moment:shortt:*:moment... */
gchar* audio_data_to_string(AudioData* dt,
			    const guint stream,
			    const guint prog,
			    const guint pid);

#endif /* AUDIODATA_H */
