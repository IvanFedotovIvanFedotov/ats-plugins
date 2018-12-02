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
"float audio_levels [6];"\
"audio_levels[0]=1.00;" \
"audio_levels[1]=0.94;" \
"audio_levels[2]=0.76;" \
"audio_levels[3]=0.58;" \
"audio_levels[4]=0.40;" \
"audio_levels[5]=0.0;"



#define COLOR_AUDIO_LEVELS \
"vec4 color_audio_levels [5];"\
"color_audio_levels[0]=vec4(1.0,0.0,0.0,1.0);" \
"color_audio_levels[1]=vec4(1.0,1.0,0.0,1.0);" \
"color_audio_levels[2]=vec4(0.0,1.0,0.0,1.0);" \
"color_audio_levels[3]=vec4(0.0,0.6,0.0,1.0);" \
"color_audio_levels[4]=vec4(0.0,0.45,0.0,1.0);" \



/*
float audio_levels [AUDIO_LEVELS+1]={1.00, 0.90, 0.84, 0.67, 0.53, 0.0};

float color_audio_levels [AUDIO_LEVELS*4]= \
{1.0,0.0,0.0,1.0, \
 1.0,0.8,0.0,1.0, \
 1.0,1.0,0.0,1.0, \
 0.0,1.0,0.0,1.0, \
 0.0,0.5,0.0,1.0};

*/


#define COLOR_PEAK "vec4(1.0,1.0,1.0,1.0)"



static const gchar *bar3_vertex_src =
    "attribute vec4 position;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = position;\n"
    "}";



static const gchar *bar3_fragment_to_up_src =
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
    "uniform float band_len;\n"
    "uniform float band_distanse;\n"
    "uniform float peak_len;\n"
//    "uniform float audio_levels["AUDIO_LEVELS_AS_STR"+1];\n"
//    "uniform vec4 color_audio_levels["AUDIO_LEVELS_AS_STR"];\n"
    "int bar_num;\n"
    "float bar_pos;\n"
    "float pos;\n"
    "float loud_average;"
    "float loud_peak;"
    "float y;"
    "void main()\n"
    "{\n"
        AUDIO_LEVELS
        COLOR_AUDIO_LEVELS
    "   float coord_x;\n"
    "   float coord_y;\n"
    "   coord_x=gl_FragCoord.x/width;\n"
    "   coord_y=gl_FragCoord.y/height;\n"
    "   pos=coord_x-bars_begin;\n"
    "   bar_pos=mod(pos,bar_step);\n"
    "   bar_num=int(pos/bar_step);\n"
    "   y=1.0-coord_y;\n"
    "   if(bar_num<0 || bar_num>=bars_num || bar_pos>bar_len || pos<0.0){"// || bar_pos+1.0>bar_len || pos<0.0){\n"
    "       gl_FragColor = bg_color;\n"
    "       return;\n"
    "   }\n"
    "   loud_average=(1.0-loud_average_arr[bar_num]);\n"
    "   loud_peak=(1.0-loud_peak_arr[bar_num]);\n"
    "   loud_peak=max(loud_peak,peak_len);"
    //"   if(loud_peak<peak_len)loud_peak=peak_len;\n"
    "   if(coord_y+peak_len>loud_peak && coord_y<loud_peak){\n"
    "       gl_FragColor ="COLOR_PEAK";\n"
    "       return;\n"
    "   }\n"
    "   if(coord_y>loud_average){\n"
    "       int index=0;"
/*  speed test - 123ms (10k shader runs (average ~300 runs)):
    "       float indexf=0.0;"
    "       indexf=2.1*indexf+1.1;"
    "       indexf=max(indexf,0.0);"
    "       index=int(indexf);"
*/
//  speed test. - 123ms:
    "       if(y>audio_levels[2]){"
    "         if(y>audio_levels[1])index=0;"
    "         else index=1;"
    "       }else{"
    "         if(y>audio_levels[3])index=2;"
    "         else {"
    "           if(y>audio_levels[4])index=3;"
    "           else index=4;"
    "         }"
    "       }"
    "       if(mod(y,band_distanse)<band_len){\n"
    "         gl_FragColor=color_audio_levels[index];\n"
    "         return;\n"
    "       }\n"
    "   }\n"
    "   gl_FragColor = bg_color;\n"
    "}";



static const gchar *bar3_fragment_to_right_src =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "int shaderright;"
    //"uniform float audio_levels["AUDIO_LEVELS_AS_STR"+1];\n"
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
    "uniform float band_len;\n"
    "uniform float band_distanse;\n"
    "uniform float peak_len;\n"
//    "uniform float audio_levels["AUDIO_LEVELS_AS_STR"+1];\n"
//    "uniform vec4 color_audio_levels["AUDIO_LEVELS_AS_STR"];\n"
    "int bar_num;\n"
    "float bar_pos;\n"
    "float pos;\n"
    "float loud_average;\n"
    "float loud_peak;\n"
    "void main()\n"
    "{\n"
        AUDIO_LEVELS
        COLOR_AUDIO_LEVELS
    "   float coord_x;\n"
    "   float coord_y;\n"
    "   coord_x=gl_FragCoord.x/width;\n"
    "   coord_y=gl_FragCoord.y/height;\n"
    "   pos=coord_y-bars_begin;\n"
    "   bar_pos=mod(pos,bar_step);\n"
    "   bar_num=int(pos/bar_step);\n"
    "   if(bar_num<0 || bar_num>=bars_num || bar_pos>bar_len || pos<0.0){\n"
    "       gl_FragColor = bg_color;\n"
    "       return;\n"
    "   }\n"
    "   loud_average=loud_average_arr[bar_num];\n"
    "   loud_peak=loud_peak_arr[bar_num];\n"
    //"   if(loud_peak<peak_len)loud_peak=peak_len;\n"
    "   loud_peak=max(loud_peak,peak_len);"
    "   if(coord_x<loud_peak && coord_x>loud_peak-peak_len){\n"
    "       gl_FragColor ="COLOR_PEAK";\n"
    "       return;\n"
    "   }\n"
    "   if(coord_x<loud_average){\n"
    "       int index=0;"
    "       if(coord_x>audio_levels[2]){"
    "         if(coord_x>audio_levels[1])index=0;"
    "         else index=1;"
    "       }else{"
    "         if(coord_x>audio_levels[3])index=2;"
    "         else {"
    "           if(coord_x>audio_levels[4])index=3;"
    "           else index=4;"
    "         }"
    "       }"
    "       if(mod(coord_x,band_distanse)<band_len){\n"
    "         gl_FragColor=color_audio_levels[index];\n"
    "         return;\n"
    "       }\n"
    "   }\n"
    "   gl_FragColor = bg_color;\n"
    "}";


void gldraw_first_init(GlDrawing *src){

  //shader_env_first_init(&src->bar_shader);

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

  src->bar_aspect=1.0/(float)audio_proceess_result->channels*0.7;
  src->band_len_percent=0.016;
  src->band_distanse_percent=0.020;
  src->peak_height_percent=0.004;

  src->bar_quads_num=audio_proceess_result->channels;

  if((float)audio_proceess_result->channels<=0)return FALSE;



  channels=(float)audio_proceess_result->channels;

  src->bar_len=src->bar_aspect;
  src->bar_step=src->bar_len+(1.0-src->bar_len*(channels))/(channels+2.0);
  src->bars_begin=(1.0-src->bar_step*channels)/2.0+(src->bar_step-src->bar_len)/2.0;
  src->bars_end=src->bars_begin+src->bar_step*channels+(src->bar_step-src->bar_len)/2.0;
  src->band_len=src->band_len_percent*2.0;
  src->band_distanse=src->band_distanse_percent*2.0;
  src->peak_len=src->peak_height_percent*2.0;


  //-------------



/*
  //free
  if(src->shader!=NULL){
    gst_object_unref(src->shader);
    src->shader=NULL;
  }

  if (src->vao){
    gl->DeleteVertexArrays (1, &src->vao);
  }

  if (src->vbo){
    gl->DeleteBuffers (1, &src->vbo);
  }

  if (src->vbo_indices){
    gl->DeleteBuffers (1, &src->vbo_indices);
  }

  src->vao=-1;
  src->vbo=-1;
  src->vbo_indices=-1;
  src->attributes_location = -1;
  memset(src->error_message, 0, error_message_size);
  //free
*/


  if(gl->GenVertexArrays){
    gl->GenVertexArrays (1, &src->vao);
    gl->BindVertexArray (src->vao);
  }

  gl->GenBuffers (1, &src->vbo);
  gl->GenBuffers (1, &src->vbo_indices);

  gl->BindBuffer (GL_ARRAY_BUFFER, src->vbo);
  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->vbo_indices);

  switch(src->draw_direction){
    case GLSOUND_BAR_DRAW_DIRECTION_TO_UP:
      src->shader = gst_gl_shader_new_link_with_stages (context, &error,
        gst_glsl_stage_new_with_string (context, GL_VERTEX_SHADER,
          GST_GLSL_VERSION_NONE,
          GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
          bar3_vertex_src),
        gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
          GST_GLSL_VERSION_NONE,
          GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
          bar3_fragment_to_up_src), NULL);
       break;
    case GLSOUND_BAR_DRAW_DIRECTION_TO_RIGHT:
      src->shader = gst_gl_shader_new_link_with_stages (context, &error,
        gst_glsl_stage_new_with_string (context, GL_VERTEX_SHADER,
          GST_GLSL_VERSION_NONE,
          GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
          bar3_vertex_src),
        gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
          GST_GLSL_VERSION_NONE,
          GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
          bar3_fragment_to_right_src), NULL);
      break;
    default:
    break;
  }

  if(!src->shader){
    int len;
    len=strlen(error->message);
    if(len<error_message_size)memcpy(src->error_message, error->message, len);
    ret=FALSE;
  }

  src->attributes_location = gst_gl_shader_get_attribute_location (src->shader, "position");
  gl->BufferData (GL_ARRAY_BUFFER, 4 * sizeof(XYZW), positions, GL_STATIC_DRAW);
  gl->BufferData (GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof (gushort), indices_quad, GL_DYNAMIC_DRAW);
  gl->VertexAttribPointer (src->attributes_location, 4, GL_FLOAT, GL_FALSE, sizeof (GLfloat) * 4, (gpointer) (gintptr) 0);
  //gl->EnableVertexAttribArray (src->attributes_location);

  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->BindBuffer (GL_ARRAY_BUFFER, 0);
  gl->BindVertexArray (0);

  return ret;
}




gboolean gldraw_render(GstGLContext * context, GlDrawing *src, loudness *audio_proceess_result)
{

  if(audio_proceess_result->channels<=0)return FALSE;
/*
  int k;
  for(k=0;k<64;k++){
    audio_proceess_result->loud_average_db[k]=1.0;
    audio_proceess_result->loud_peak_db[k]=1.0;
  }
*/
  const GstGLFuncs *gl = context->gl_vtable;
  if(gl->GenVertexArrays){
    gl->BindVertexArray (src->vao);
  }
  gl->BindBuffer (GL_ARRAY_BUFFER, src->vbo);
  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->vbo_indices);
  gst_gl_shader_use (src->shader);
  gl->EnableVertexAttribArray (src->attributes_location);


  //gst_gl_shader_set_uniform_1fv(src->shader, "audio_levels", AUDIO_LEVELS+1, audio_levels);
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
  gst_gl_shader_set_uniform_1f(src->shader, "band_len",src->band_len);
  gst_gl_shader_set_uniform_1f(src->shader, "band_distanse",src->band_distanse);
  gst_gl_shader_set_uniform_1f(src->shader, "peak_len",src->peak_len);
//  gst_gl_shader_set_uniform_1fv(src->shader, "audio_levels", MAX_CHANNELS+1, audio_levels);
//  gst_gl_shader_set_uniform_4fv(src->shader, "color_audio_levels", MAX_CHANNELS*4, color_audio_levels);


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










