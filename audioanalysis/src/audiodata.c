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

void param_reset (AudioParams* p) {
        memset(p, 0, sizeof(AudioParams));
        p->counter_moment = 0;
        p->counter_shortt = 0;
}
void param_avg (AudioParams* p) {
        p->moment.avg /= (double) p->counter_moment;
        p->shortt.avg /= (double) p->counter_shortt;
}
void param_add_shortt  (AudioParams* p, double v) {
        if (p->counter_shortt == 0) {
                p->shortt.avg = v;
                p->shortt.max = v;
                p->shortt.min = v;
                p->counter_shortt++;
                return;
        }
        if (v > p->shortt.max) {
                p->shortt.max = v;
        } else if (v < p->shortt.min) {
                p->shortt.min = v;
        }
        p->shortt.avg += v;
        p->counter_shortt++;
}

void param_add_moment (AudioParams* p, double v) {
        if (p->counter_moment == 0) {
                p->moment.avg = v;
                p->moment.max = v;
                p->moment.min = v;
                p->counter_moment++;
                return;
        }
        if (v > p->moment.max) {
                p->moment.max = v;
        } else if (v < p->moment.min) {
                p->moment.min = v;
        }
        p->moment.avg += v;
        p->counter_moment++;
}   
