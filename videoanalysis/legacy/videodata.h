/* videodata.h
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
#ifndef VIDEODATA_H
#define VIDEODATA_H

#include <glib.h>
#include <gst/gst.h>

typedef enum { BLACK, LUMA, FREEZE, DIFF, BLOCKY, PARAM_NUMBER } PARAMETER;

const char* param_to_string (PARAMETER);

typedef struct __Param       Param;
typedef struct __VideoParams VideoParams;

struct __Param {
  float min;
  float max;
  float avg;
};

struct __VideoParams {
  Param    param[PARAM_NUMBER];
  gboolean empty[PARAM_NUMBER];
};

Param* param_of_video_params (VideoParams*, PARAMETER);
void param_reset (VideoParams*);
void param_avg (VideoParams*, float);
void param_add (VideoParams*, PARAMETER, float);

#endif /* VIDEOANALYSIS_API_H */
