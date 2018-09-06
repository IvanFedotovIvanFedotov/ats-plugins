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

#include "videodata.h"
#include <malloc.h>
#include <string.h>

Param*
param_of_video_params (VideoParams* vp, PARAMETER p)
{
        return &vp->param[p];
}

const char*
param_to_string (PARAMETER p)
{
        switch (p) {
        case BLACK: {
                return "black";
                break;
        }
        case LUMA: {
                return "luma";
                break;
        }
        case FREEZE: {
                return "freeze";
                break;
        }
        case DIFF: {
                return "diff";
                break;
        }
        case BLOCKY: {
                return "blocky";
                break;
        }
        default:
                return "unknown";
                break;
        }
}

void
param_reset (VideoParams* p) {
        memset(p, 0, sizeof(VideoParams));
        for (int i = 0; i < PARAM_NUMBER; i++) {
                p->empty[i] = TRUE;
        }
}

void
param_avg (VideoParams* p, float sz) {
        for (int i = 0; i < PARAM_NUMBER; i++) {
                p->param[i].avg /= sz;
        }
}

void
param_add (VideoParams* p, PARAMETER d, float v) {
        Param* par = param_of_video_params(p, d);
        if (p->empty[d]) {
                par->max = v;
                par->min = v;
                par->avg = v;
                p->empty[d] = FALSE;
                return;
        }
        if (v > par->max) {
                par->max = v;
        } else if (v < par->min) {
                par->min = v; 
        }

        par->avg += v;
}
