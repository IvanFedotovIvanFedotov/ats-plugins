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

#include <gst/gst.h>
#include "gstglsoundbar.h"
#include "audiosamplesbuf.h"

#include "shaderenv.h"

#include <stdlib.h>
#include <math.h>





float audio_levels[AUDIO_LEVELS+1]={
    1.0,
    0.90,//0 dB
    0.84,//-6 dB
    0.67,//-23 dB
    0.53,//-37 dB
    0.0
};

#define COLOR_AUDIO_LEVEL_0 "vec4(1.0,0.0,0.0,1.0)"
#define COLOR_AUDIO_LEVEL_1 "vec4(1.0,0.8,0.0,1.0)"
#define COLOR_AUDIO_LEVEL_2 "vec4(1.0,1.0,0.0,1.0)"
#define COLOR_AUDIO_LEVEL_3 "vec4(0.0,1.0,0.0,1.0)"
#define COLOR_AUDIO_LEVEL_4 "vec4(0.0,0.5,0.0,1.0)"

#define COLOR_PEAK "vec4(1.0,1.0,1.0,1.0)"



static const gchar *bar3_vertex_src =
    "attribute vec4 position;\n"
    "varying vec2 out_uv;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = position;\n"
    "   out_uv = position.xy;\n"
    "}";



static const gchar *bar3_fragment_to_up_src =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "int shaderup;"
    "uniform float audio_levels_in_pixels["AUDIO_LEVELS_AS_STR"+1];\n"
    "uniform int  bars_num;\n"
    "uniform vec4 bg_color;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "uniform float loud_average[64];"
    "uniform float loud_peak[64];"
    "uniform float bars_begin_pix;\n"
    "uniform float bars_end_pix;\n"
    "uniform float bar_len_pix;\n"
    "uniform float bar_step_pix;\n"
    "uniform float band_len_pix;\n"
    "uniform float band_distanse_pix;\n"
    "uniform float peak_len_pix;\n"
    "int bar_num;\n"
    "float bar_pos;\n"
    "float pos;\n"
    "float loud_average_pix;"
    "float loud_peak_pix;"
    "float y;"
    "varying vec2 out_uv;\n"
    "void main()\n"
    "{\n"
    "   pos=gl_FragCoord.x-bars_begin_pix;\n"
    "   bar_pos=mod(pos,bar_step_pix);\n"
    "   bar_num=int(pos/bar_step_pix);\n"
    "   y=height-gl_FragCoord.y;\n"
    "   if(bar_num<0 || bar_num>=bars_num || bar_pos+1.0>bar_len_pix || pos<0.0){\n"
    "       gl_FragColor = bg_color;\n"
    "       return;\n"
    "   }\n"
    "   loud_average_pix=(1.0-loud_average[bar_num])*height;\n"
    "   loud_peak_pix=(1.0-loud_peak[bar_num])*height;\n"
    "   if(loud_peak_pix<peak_len_pix)loud_peak_pix=peak_len_pix;\n"
    "   if(gl_FragCoord.y+peak_len_pix>loud_peak_pix && gl_FragCoord.y<loud_peak_pix){\n"
    "       gl_FragColor ="COLOR_PEAK";\n"
    "       return;\n"
    "   }\n"
    "   if(gl_FragCoord.y>loud_average_pix){\n"
    "       if(gl_FragCoord.y<audio_levels_in_pixels[1]){\n"
    "         if(mod(y,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_0";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "       if(gl_FragCoord.y<audio_levels_in_pixels[2]){\n"
    "         if(mod(y,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_1";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "       if(gl_FragCoord.y<audio_levels_in_pixels[3]){\n"
    "         if(mod(y,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_2";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "       if(gl_FragCoord.y<audio_levels_in_pixels[4]){\n"
    "         if(mod(y,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_3";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "       if(gl_FragCoord.y<audio_levels_in_pixels[5]){\n"
    "         if(mod(y,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_4";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "   }\n"
    "   gl_FragColor = bg_color;\n"
    "}";



static const gchar *bar3_fragment_to_right_src =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "int shaderright;"
    "uniform float audio_levels_in_pixels["AUDIO_LEVELS_AS_STR"+1];\n"
    "uniform int  bars_num;\n"
    "uniform vec4 bg_color;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "uniform float loud_average[64];"
    "uniform float loud_peak[64];"
    "uniform float bars_begin_pix;\n"
    "uniform float bars_end_pix;\n"
    "uniform float bar_len_pix;\n"
    "uniform float bar_step_pix;\n"
    "uniform float band_len_pix;\n"
    "uniform float band_distanse_pix;\n"
    "uniform float peak_len_pix;\n"
    "int bar_num;\n"
    "float bar_pos;\n"
    "float pos;\n"
    "float loud_average_pix;\n"
    "float loud_peak_pix;\n"
    "varying vec2 out_uv;\n"
    "void main()\n"
    "{\n"
    "   pos=gl_FragCoord.y-bars_begin_pix;\n"
    "   bar_pos=mod(pos,bar_step_pix);\n"
    "   bar_num=int(pos/bar_step_pix);\n"
    "   if(bar_num<0 || bar_num>=bars_num || bar_pos+1.0>bar_len_pix || pos<0.0){\n"
    "       gl_FragColor = bg_color;\n"
    "       return;\n"
    "   }\n"
    "   loud_average_pix=loud_average[bar_num]*width;\n"
    "   loud_peak_pix=loud_peak[bar_num]*width;\n"
    "   if(loud_peak_pix<peak_len_pix)loud_peak_pix=peak_len_pix;\n"
    "   if(gl_FragCoord.x<loud_peak_pix && gl_FragCoord.x>loud_peak_pix-peak_len_pix){\n"
    "       gl_FragColor ="COLOR_PEAK";\n"
    "       return;\n"
    "   }\n"
    "   if(gl_FragCoord.x<loud_average_pix){\n"
    "       if(gl_FragCoord.x>audio_levels_in_pixels[1]){\n"
    "         if(mod(gl_FragCoord.x,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_0";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "       if(gl_FragCoord.x>audio_levels_in_pixels[2]){\n"
    "         if(mod(gl_FragCoord.x,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_1";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "       if(gl_FragCoord.x>audio_levels_in_pixels[3]){\n"
    "         if(mod(gl_FragCoord.x,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_2";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "       if(gl_FragCoord.x>audio_levels_in_pixels[4]){\n"
    "         if(mod(gl_FragCoord.x,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_3";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "       if(gl_FragCoord.x>audio_levels_in_pixels[5]){\n"
    "         if(mod(gl_FragCoord.x,band_distanse_pix)<band_len_pix){\n"
    "           gl_FragColor="COLOR_AUDIO_LEVEL_4";\n"
    "           return;\n"
    "         }\n"
    "       }\n"
    "   }\n"
    "   gl_FragColor = bg_color;\n"
    "}";


gboolean gldraw_init (GstGLContext * context, GlDrawing *src, ResultData *audio_proceess_result,
                      gint width, gint height,
                      gint direction,
                      gfloat bar_aspect,
                      gfloat bar_risc_len_percent,
                      gfloat bar_risc_step_percent,
                      gfloat peak_height_percent,
                      gboolean bar_aspect_auto,
                      gfloat bg_color_r,
                      gfloat bg_color_g,
                      gfloat bg_color_b,
                      gfloat bg_color_a)
{

  float channels;
  //горизонтально относительно вертикального направления бара
  float horizontal_size_pix;
  //вертикально относительно вертикального направления бара
  float vertical_size_pix;

  src->bg_color.R=bg_color_r;
  src->bg_color.G=bg_color_g;
  src->bg_color.B=bg_color_b;
  src->bg_color.A=bg_color_a;

  src->bar_aspect_auto=bar_aspect_auto;
  src->pixel_width=width;
  src->pixel_height=height;
  src->draw_direction=direction;
  //src->draw_direction=1;

  src->bar_aspect=bar_aspect;
  if(src->bar_aspect_auto==TRUE && audio_proceess_result->channels>0){
    src->bar_aspect=1.0/(float)audio_proceess_result->channels*0.7;
  }

  src->band_len_percent=bar_risc_len_percent;
  src->band_distanse_percent=bar_risc_step_percent;
  src->peak_height_percent=peak_height_percent;

  src->bar_quads_num=audio_proceess_result->channels;

  if((float)audio_proceess_result->channels<=0)return FALSE;

  switch(src->draw_direction){
    case GLSOUND_BAR_DRAW_DIRECTION_TO_UP:
       shader_env_create(context, &src->bar_shader, bar3_vertex_src, bar3_fragment_to_up_src);
       horizontal_size_pix=src->pixel_width;
       vertical_size_pix=src->pixel_height;
       for(int i=0;i<AUDIO_LEVELS+1;i++){
         src->audio_levels_in_pixels[i]=src->pixel_height*(1.0-audio_levels[i]);
       }
       break;
    case GLSOUND_BAR_DRAW_DIRECTION_TO_RIGHT:
       shader_env_create(context, &src->bar_shader, bar3_vertex_src, bar3_fragment_to_right_src);
       horizontal_size_pix=src->pixel_height;
       vertical_size_pix=src->pixel_width;
       for(int i=0;i<AUDIO_LEVELS+1;i++){
         src->audio_levels_in_pixels[i]=src->pixel_width*audio_levels[i];
       }
      break;
    default:
    break;
  }

  channels=(float)audio_proceess_result->channels;

  src->bar_len_pix=round(src->bar_aspect*horizontal_size_pix);
  src->bar_step_pix=round(src->bar_len_pix+(horizontal_size_pix-src->bar_len_pix*(channels))/(channels+2.0));
  src->bars_begin_pix=round((horizontal_size_pix-src->bar_step_pix*channels)/2.0+(src->bar_step_pix-src->bar_len_pix)/2.0);
  src->bars_end_pix=round(src->bars_begin_pix+src->bar_step_pix*channels+(src->bar_step_pix-src->bar_len_pix)/2.0);
  src->band_len_pix=round(src->band_len_percent*vertical_size_pix);
  src->band_distanse_pix=round(src->band_distanse_percent*vertical_size_pix);
  src->peak_len_pix=round(src->peak_height_percent*vertical_size_pix);

  return TRUE;
}

gboolean gldraw_render(GstGLContext * context, GlDrawing *src, ResultData *audio_proceess_result)
{

  if(audio_proceess_result->channels<=0)return FALSE;

  shader_env_bind(context, &src->bar_shader);

  gst_gl_shader_set_uniform_1fv(src->bar_shader.shader, "audio_levels_in_pixels", AUDIO_LEVELS+1, src->audio_levels_in_pixels);
  gst_gl_shader_set_uniform_1i(src->bar_shader.shader, "bars_num", src->bar_quads_num);
  gst_gl_shader_set_uniform_4f(src->bar_shader.shader, "bg_color", src->bg_color.R, src->bg_color.G, src->bg_color.B, src->bg_color.A);
  gst_gl_shader_set_uniform_1f(src->bar_shader.shader, "width", src->pixel_width);
  gst_gl_shader_set_uniform_1f(src->bar_shader.shader, "height", src->pixel_height);
  gst_gl_shader_set_uniform_1fv(src->bar_shader.shader, "loud_average", src->bar_quads_num, (float *)audio_proceess_result->loud_average);
  gst_gl_shader_set_uniform_1fv(src->bar_shader.shader, "loud_peak", src->bar_quads_num, (float *)audio_proceess_result->loud_peak);
  gst_gl_shader_set_uniform_1f(src->bar_shader.shader, "bars_begin_pix",src->bars_begin_pix);
  gst_gl_shader_set_uniform_1f(src->bar_shader.shader, "bars_end_pix",src->bars_end_pix);
  gst_gl_shader_set_uniform_1f(src->bar_shader.shader, "bar_len_pix",src->bar_len_pix);
  gst_gl_shader_set_uniform_1f(src->bar_shader.shader, "bar_step_pix",src->bar_step_pix);
  gst_gl_shader_set_uniform_1f(src->bar_shader.shader, "band_len_pix",src->band_len_pix);
  gst_gl_shader_set_uniform_1f(src->bar_shader.shader, "band_distanse_pix",src->band_distanse_pix);
  gst_gl_shader_set_uniform_1f(src->bar_shader.shader, "peak_len_pix",src->peak_len_pix);

  shader_env_draw(context, &src->bar_shader);
  shader_env_unbind(context, &src->bar_shader);

  return TRUE;

}

void gldraw_close (GstGLContext * context, GlDrawing *src, ResultData *audio_proceess_result)
{

  shader_env_free(context, &src->bar_shader);

}










