/*
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2002,2007 David A. Schleef <ds@schleef.org>
 * Copyright (C) 2008 Julien Isorce <julien.isorce@gmail.com>
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
 * Copyright (C) 2018 NIITV. Ivan Fedotov<ivanfedotovmail@yandex.ru>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef __GLDRAWING_H__
#define __GLDRAWING_H__

#include "shaderenv.h"


//2 переменные должны быть оодинаковыми
#define AUDIO_LEVELS (5)
#define AUDIO_LEVELS_AS_STR "5"

typedef struct _RECT_INT RECT_INT;
struct _RECT_INT{

  gint x1,y1,x2,y2;

};

typedef struct _RECT_FLOAT RECT_FLOAT;
struct _RECT_FLOAT{

  float x1,y1,x2,y2;

};

typedef struct _COLOR_COMPONENTS COLOR_COMPONENTS;
struct _COLOR_COMPONENTS{

  float R,G,B,A;

};

typedef struct _GlDrawing GlDrawing;
struct _GlDrawing
{

  struct ShaderEnv bar_shader;
  gint bar_quads_num;
  //На примере вертикального бара, рисуемого снизу вверх
  //При повороте переменные в данном блоке всеравно считаются относительно верха бара
  //Отношение ширины к длине 1 бара
  float bar_aspect;
  //Процент высоты риски к высоте бара
  float band_len_percent;
  //Процент расстояния между рисками к высоте бара
  float band_distanse_percent;
  //высота пика в процентах от высоты бара
  float peak_height_percent;

  float bars_begin_pix;
  float bars_end_pix;
  float bar_len_pix;
  float bar_step_pix;
  float band_len_pix;
  float band_distanse_pix;
  float peak_len_pix;

  gint draw_direction;

  gint pixel_width;
  gint pixel_height;

  COLOR_COMPONENTS bg_color;

  float audio_levels_in_pixels[AUDIO_LEVELS+1];

  gchar error_message[error_message_size];

};

void gldraw_first_init(GlDrawing *src);
gboolean gldraw_init (GstGLContext * context, GlDrawing *src, ResultData *audio_proceess_result,
                      gint width, gint height,
                      gint direction,
                      gfloat bg_color_r,
                      gfloat bg_color_g,
                      gfloat bg_color_b,
                      gfloat bg_color_a);

gboolean gldraw_render(GstGLContext * context, GlDrawing *src, ResultData *audio_proceess_result);
void gldraw_close (GstGLContext * context, GlDrawing *src, ResultData *audio_proceess_result);

#endif //__GLDRAWING_H__


