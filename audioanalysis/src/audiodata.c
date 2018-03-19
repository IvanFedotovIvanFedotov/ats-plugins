/* videodata.c
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

#include "audiodata.h"
#include <malloc.h>
#include <string.h>

AudioData*
audio_data_new(guint fr)
{
  if (fr == 0) return NULL;

  AudioData* rval;
  rval = (AudioData*) malloc(sizeof(AudioData));
  rval->period = fr;
  rval->current = 0;
  rval->data = (AudioParams*) malloc(sizeof(AudioParams) * fr);
  return rval;
}

void
audio_data_delete(AudioData* dt)
{
  free(dt->data);
  dt->data = NULL;
  free(dt);
  dt = NULL;
  return;
}

int
audio_data_append(AudioData* dt,
		  AudioParams* par)
{
  if(dt->current == dt->period) return -1;
  unsigned int i = dt->current;
  dt->data[i] = *par;
  dt->current++;
  return 0;
}

gboolean
audio_data_is_full(AudioData* dt)
{
  if (dt->current == dt->period) return TRUE;
  return FALSE;
}


gpointer
audio_data_dump(AudioData* dt, gsize* sz) {
        if (sz == NULL) return NULL;
        
        *sz = dt->current;
        AudioParams* buf = (AudioParams*) malloc(sizeof(AudioParams) * (*sz));
        memcpy(buf, dt->data, (sizeof(AudioParams) * (*sz)));        
        return buf;
}

/* a(stream):(prog):(pid):*:moment:shortt:*:moment... */
gchar*
audio_data_to_string(AudioData* dt,
		     const guint stream,
		     const guint prog,
		     const guint pid)
{
  guint i;
  gchar* string;
  string = g_strdup_printf("a%d:%d:%d", stream, prog, pid);
  for (i = 0; i < dt->period; i++) {
    gchar* pr_str = string;
    gchar* tmp = g_strdup_printf(":*:%f:%f",
				 dt->data[i].moment,
				 dt->data[i].shortt);
    string = g_strconcat(pr_str, tmp, NULL);
    g_free(tmp);
    g_free(pr_str);
  }
  return string;
}
