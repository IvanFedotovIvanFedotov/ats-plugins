/* test
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

#include <stdlib.h>
#include <math.h>

//0
//-6 dB
//-24
//-42
//-60
#define AUDIO_LEVELS \
"const float audio_levels [6]=float[6](1.00, 0.94, 0.76, 0.58, 0.40, 0.0);"

#define COLOR_AUDIO_LEVELS \
"const vec4 color_audio_levels [5]=vec4[5]("\
"vec4(1.0,0.0,0.0,1.0)," \
"vec4(1.0,1.0,0.0,1.0)," \
"vec4(0.0,1.0,0.0,1.0)," \
"vec4(0.0,0.6,0.0,1.0)," \
"vec4(0.0,0.45,0.0,1.0)" \
");"


#define COLOR_PEAK "vec4(1.0,1.0,1.0,1.0)"


#define BAND_LEN "float band_len=0.032;"
#define BAND_DISTANSE "float band_distanse=0.040;"
#define PEAK_SIZE "float peak_size=0.008;"
#define BAR_ASPECT 0.7


static const gchar *bar_vertex_src =
    "#version 120\n"
    "attribute vec4 position;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = position;\n"
    "}";



static const gchar *bar_fragment_src =
    "#version 120\n"
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "int shaderup;"
    "uniform int  bars_num;\n"
    "uniform vec4 bg_color;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "uniform float loud_average_arr["MAX_CHANNELS_AS_STR"];"
    "uniform float loud_peak_arr["MAX_CHANNELS_AS_STR"];"
    "uniform float bars_begin;\n"
    "uniform float bars_end;\n"
    "uniform float bar_len;\n"
    "uniform float bar_step;\n"
    "uniform int direction;\n"//0-vertical
    BAND_LEN
    BAND_DISTANSE
    PEAK_SIZE
    AUDIO_LEVELS
    COLOR_AUDIO_LEVELS
    "int bar_num;\n"
    "float bar_pos;\n"
    "float pos;\n"
    "float loud_average;"
    "float loud_peak;"
    "float y;"
    "void main()\n"
    "{\n"
    "   float coord_x;\n"
    "   float coord_y;\n"
    "   coord_x=gl_FragCoord.x/width;\n"
    "   coord_y=gl_FragCoord.y/height;\n"
    "   if(direction==0){"
    "     pos=coord_x-bars_begin;\n"
    "   }"
    "   else{"
    "     pos=coord_y-bars_begin;\n"
    "   }"
/*
  //test Eugenii
    "   float bar_step1=(bars_end-bars_begin)/float(bars_num);"
    "   float bar_len1= 0.5*bar_step1;"
    "   bar_pos=mod(pos,bar_step1);\n"
    "   bar_num=int(pos/bar_step1);\n"
    "   y=1.0-coord_y;\n"
    "   if(bar_num<0 || bar_num>=bars_num || bar_pos>bar_len1 || pos<0.0){"
    "       gl_FragColor = bg_color;\n"
    "       return;\n"
    "   }\n"
*/

/*
  //test rounds
    "   float bar_step1=round((bars_end-bars_begin)/float(bars_num)*width)/width;"
    "   float bar_len1=round( 0.5*bar_step1*width)/width;"
    "   bar_pos=mod(pos,bar_step1);\n"
    "   bar_num=int(pos/bar_step1);\n"
    "   y=1.0-coord_y;\n"
    "   if(bar_num<0 || bar_num>=bars_num || bar_pos>bar_len1 || pos<0.0){"
    "       gl_FragColor = bg_color;\n"
    "       return;\n"
    "   }\n"
*/

  // shader work
    "   bar_pos=mod(pos,bar_step);\n"
    "   bar_num=int(pos/bar_step);\n"
    "   y=1.0-coord_y;\n"
    "   if(bar_num<0 || bar_num>=bars_num || bar_pos>bar_len || pos<0.0){"
    "       gl_FragColor = bg_color;\n"
    "       return;\n"
    "   }\n"
  // shader work

    "   if(direction==0){"
    "      loud_average=(1.0-loud_average_arr[bar_num]);\n"
    "      loud_peak=(1.0-loud_peak_arr[bar_num]);\n"
    "      loud_peak=max(loud_peak,peak_size);"
    "      if(coord_y+peak_size>loud_peak && coord_y<loud_peak){\n"
    "          gl_FragColor ="COLOR_PEAK";\n"
    "          return;\n"
    "      }\n"
    "      if(coord_y>loud_average){\n"
    "          int index=0;"
    "          index=int(floor(-5.556*y+6.222));"
    "          index=min(index,4);"
    "          if(mod(y,band_distanse)<band_len){\n"
    "            gl_FragColor=color_audio_levels[index];\n"
    "            return;\n"
    "          }\n"
    "      }\n"
    "   }"
    "   else{"
    "      loud_average=loud_average_arr[bar_num];\n"
    "      loud_peak=loud_peak_arr[bar_num];\n"
    "      loud_peak=max(loud_peak,peak_size);"
    "      if(coord_x<loud_peak && coord_x>loud_peak-peak_size){\n"
    "          gl_FragColor ="COLOR_PEAK";\n"
    "          return;\n"
    "      }\n"
    "      if(coord_x<loud_average){\n"
    "          int index=0;"
    "          index=int(floor(-5.556*coord_x+6.222));"
    "          index=min(index,4);"
    "          if(mod(coord_x,band_distanse)<band_len){\n"
    "            gl_FragColor=color_audio_levels[index];\n"
    "            return;\n"
    "          }\n"
    "      }\n"
    "   }"
    "   gl_FragColor = bg_color;\n"
    "}";





void gldraw_first_init(GlDrawing *src){

  src->shader=NULL;
  src->vao=0;
  src->vbo=0;
  src->vbo_indices=0;
  src->attributes_location = 0;
  memset(src->error_message, 0, error_message_size);

}


void setBGColor(GlDrawing *src, unsigned int color){

  src->bg_color.R=(float)((color & 0x00ff0000)>>16)/255.0;
  src->bg_color.G=(float)((color & 0x0000ff00)>>8)/255.0;
  src->bg_color.B=(float)((color & 0x000000ff))/255.0;
  src->bg_color.A=(float)((color & 0xff000000)>>24)/255.0;

}


gboolean gldraw_init (GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result,
                      int width, int height,
                      int direction,
                      float bg_color_r,
                      float bg_color_g,
                      float bg_color_b,
                      float bg_color_a)
{

  float channels;
  //горизонтально относительно вертикального направления бара
  float horizontal_size_pix;
  //вертикально относительно вертикального направления бара
  float vertical_size_pix;

  int len;

  const GstGLFuncs *gl = context->gl_vtable;

  GError *error = NULL;

  gboolean ret;

  ret=TRUE;

  memset(src->error_message, 0, error_message_size);

  src->bg_color.R=bg_color_r;
  src->bg_color.G=bg_color_g;
  src->bg_color.B=bg_color_b;
  src->bg_color.A=bg_color_a;

  src->width=width;
  src->height=height;
  src->draw_direction=direction;

  if(audio_proceess_result->channels<=0)return FALSE;

  src->bar_aspect=1.0/(float)audio_proceess_result->channels*BAR_ASPECT;
  src->bar_quads_num=audio_proceess_result->channels;

  if((float)audio_proceess_result->channels<=0)return FALSE;

  channels=(float)audio_proceess_result->channels;

  src->bar_len=src->bar_aspect;
  src->bar_step=src->bar_len+(1.0-src->bar_len*(channels))/(channels+2.0);
  src->bars_begin=(1.0-src->bar_step*channels)/2.0+(src->bar_step-src->bar_len)/2.0;
  src->bars_end=src->bars_begin+src->bar_step*channels+(src->bar_step-src->bar_len)/2.0;

  if(gl->GenVertexArrays){
    gl->GenVertexArrays (1, &src->vao);
    gl->BindVertexArray (src->vao);
  }

  gl->GenBuffers (1, &src->vbo);
  gl->GenBuffers (1, &src->vbo_indices);

  gl->BindBuffer (GL_ARRAY_BUFFER, src->vbo);
  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->vbo_indices);

  src->shader = gst_gl_shader_new_link_with_stages (context, &error,
    gst_glsl_stage_new_with_string (context, GL_VERTEX_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      bar_vertex_src),
    gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      bar_fragment_src), NULL);

  if(!src->shader){
    int len;
    len=strlen(error->message);
    if(len>error_message_size)len=error_message_size;
    memcpy(src->error_message, error->message, len);
    ret=FALSE;
  }

  src->attributes_location = gst_gl_shader_get_attribute_location (src->shader, "position");
  gl->BufferData (GL_ARRAY_BUFFER, 4 * sizeof(XYZW), positions, GL_STATIC_DRAW);
  gl->BufferData (GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof (gushort), indices_quad, GL_DYNAMIC_DRAW);
  gl->VertexAttribPointer (src->attributes_location, 4, GL_FLOAT, GL_FALSE, sizeof (GLfloat) * 4, (gpointer) (gintptr) 0);

  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->BindBuffer (GL_ARRAY_BUFFER, 0);
  gl->BindVertexArray (0);

  return ret;
}


gboolean gldraw_render(GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result)
{

  if(audio_proceess_result->channels<=0)return FALSE;

  const GstGLFuncs *gl = context->gl_vtable;
  if(gl->GenVertexArrays){
    gl->BindVertexArray (src->vao);
  }
  gl->BindBuffer (GL_ARRAY_BUFFER, src->vbo);
  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->vbo_indices);
  gst_gl_shader_use (src->shader);
  gl->EnableVertexAttribArray (src->attributes_location);

  gst_gl_shader_set_uniform_1i(src->shader, "bars_num", src->bar_quads_num);
  gst_gl_shader_set_uniform_4f(src->shader, "bg_color", src->bg_color.R, src->bg_color.G, src->bg_color.B, src->bg_color.A);
  gst_gl_shader_set_uniform_1f(src->shader, "width", src->width);
  gst_gl_shader_set_uniform_1f(src->shader, "height", src->height);
  gst_gl_shader_set_uniform_1fv(src->shader, "loud_average_arr", src->bar_quads_num, (float *)audio_proceess_result->loud_average_db);
  gst_gl_shader_set_uniform_1fv(src->shader, "loud_peak_arr", src->bar_quads_num, (float *)audio_proceess_result->loud_peak_db);
  gst_gl_shader_set_uniform_1f(src->shader, "bars_begin",src->bars_begin);
  gst_gl_shader_set_uniform_1f(src->shader, "bars_end",src->bars_end);
  gst_gl_shader_set_uniform_1f(src->shader, "bar_len",src->bar_len);
  gst_gl_shader_set_uniform_1f(src->shader, "bar_step",src->bar_step);
  gst_gl_shader_set_uniform_1i(src->shader, "direction", src->draw_direction);

  gl->DrawElements (GL_TRIANGLES, 6 , GL_UNSIGNED_SHORT,(gpointer) (gintptr) 0);

  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->BindBuffer (GL_ARRAY_BUFFER, 0);
  gl->DisableVertexAttribArray (src->attributes_location);
  if(gl->GenVertexArrays){
    gl->BindVertexArray(0);
  }
  gst_gl_context_clear_shader(context);

  return TRUE;

}

void gldraw_close (GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result)
{

  const GstGLFuncs *gl = context->gl_vtable;

  if(src->shader!=NULL){
    gst_object_unref(src->shader);
    src->shader=NULL;
  }

  gl->DeleteVertexArrays (1, &src->vao);
  gl->DeleteBuffers (1, &src->vbo);
  gl->DeleteBuffers (1, &src->vbo_indices);

  src->vao=0;
  src->vbo=0;
  src->vbo_indices=0;
  src->attributes_location = 0;
  memset(src->error_message, 0, error_message_size);

}










