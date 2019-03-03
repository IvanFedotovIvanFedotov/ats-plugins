/*

gst-launch-1.0 gldisplayerrors set-errors=0x07 ! glimagesink

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
#include "gldrawing.h"

#include <stdlib.h>
#include <math.h>



//gst-launch-1.0 gldisplayerrors set_errors=0x05 ! glimagesink


//--------------------

//#include <GL/gl.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define UNUSED(a) (void)a
#define LEN(a) (sizeof(a)/sizeof(a)[0])

/*
 * Nuklear - 1.40.8 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2017 by Micha Mettke
 * emscripten from 2016 by Chris Willcocks
 * OpenGL ES 2.0 from 2017 by Dmitry Hrabrov a.k.a. DeXPeriX
 */

#include <string.h>
#include <time.h>

struct nk_sdl_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint vbo, ebo, vao;

    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
    GLsizei vs;
    size_t vp, vt, vc;
};

struct nk_sdl_vertex {
    GLfloat position[2];
    GLfloat uv[2];
    nk_byte col[4];
};

static struct nk_custom {
    //SDL_Window *win;
    struct nk_sdl_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;

    struct nk_buffer vbuf, ebuf;

    int vertices[MAX_VERTEX_MEMORY];
    int elements[MAX_ELEMENT_MEMORY];

};// sdl;

#define NK_SHADER_VERSION "#version 150\n"

typedef struct
{
  gfloat X, Y;
}XY2;

/* *INDENT-OFF* */
static const GLfloat positions2[] = {
     -1.0,  1.0,
      1.0,  1.0,
      1.0, -1.0,
     -1.0, -1.0
};

//----------------





void setBGColor(GlDrawing *src, unsigned int color){


}



const nk_rune nk_font_russian_glyph_ranges[] = {
  0x0020, 0x00FF,
  0x0400, 0x04FF,
  /*0x2200, 0x22FF, // Mathematical Operators
  0x0370, 0x03FF, // Greek and Coptic
  */
  0
};


static const GLchar *vertex_shader =
        NK_SHADER_VERSION
        "uniform mat4 ProjMtx;\n"
        "uniform float height;"
        "attribute vec2 Position;\n"
        "attribute vec2 TexCoord;\n"
        "attribute vec4 Color;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main() {\n"
        "   vec4 pos4;"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   pos4 = ProjMtx * vec4(Position.x,height-Position.y, 0.0, 1.0);\n"
        //"   pos4 = ProjMtx * vec4(Position.x,Position.y, 0.0, 1.0);\n"
        "   gl_Position = vec4(pos4.x,pos4.y,pos4.z,pos4.w);\n"
        "   return;"
        //"   gl_Position = vec4(Position,0.0,1.0);\n"
        "   vec2 pos;"
        "   pos=Position;"
        "   pos=pos/250.0;"//-vec2(1.0,1.0);"
        "   gl_Position = vec4(pos,0.0,1.0);\n"
        "}\n";

static const GLchar *fragment_shader =
        NK_SHADER_VERSION
        //"layout(origin_upper_left) in vec4 gl_FragCoord;"
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
         "#endif\n"
        //"precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main(){\n"
        //"   gl_FragColor=vec4(1.0,0.5,0.0,1.0)+Frag_Color;"
        "   gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV);\n"
        "}\n";



ErrorProperties errors_properties[all_errors_count]=
{
    0.0,0.0,0.0,255,  0.0,255,0.0,255,  255,255,255,160,  0,  ERROR_CODE_ERROR_NONE,           "0 Dummy Error",  //Dummy
    0.0,0.0,0.0,255,  255,0.0,0.0,255,  255,255,255,160,  3,  ERROR_CODE_ERROR_CAPTION1,       "1 Error русский текст full text caption 1",
    0.0,0.0,0.0,255,  255,100,0.0,255,  255,255,255,160,  3,  ERROR_CODE_ERROR_CAPTION2,       "2 Error русский текст full text caption 2",
    0.0,0.0,0.0,255,  255,160,0.0,255,  255,255,255,160,  2,  ERROR_CODE_WARNING_CAPTION1,     "3 Warning русский текст full text caption 1",
    0.0,0.0,0.0,255,  255,200,0.0,255,  255,255,255,160,  2,  ERROR_CODE_WARNING_CAPTION2,     "4 Warning русский текст full text caption 2",
    0.0,0.0,0.0,255,  255,255,0.0,255,  255,255,255,160,  1,  ERROR_CODE_MESSAGE_CAPTION1,     "5 Message русский текст full text caption 1",

};






double getCurrentTime(){

  struct timespec tms;
  if(clock_gettime(CLOCK_REALTIME,&tms))return 0.0;
  return (double)tms.tv_sec+((double)(tms.tv_nsec))/1000000000.0;

}



//this_enabled=1 = create line,set flags
//this_enabled=0 = remove line
//this_enabled=-1 = non change, auto set show time, no set flags
void set_text_line_flags(GlDrawing *src, TextRect *text_line,int error_code,int this_enabled,int show_time,double time_val){

      if(this_enabled==-1){
        if(time_val-text_line->error_last_time>src->time_delta_continous_error_min){
          text_line->show_time=0;
        }
        text_line->error_last_time=time_val;
      }
      if(this_enabled==0){
        text_line->this_enabled=0;
      }
      if(this_enabled==1){
        text_line->this_enabled=1;
        text_line->error_last_time=time_val;
        text_line->error_create_time=time_val;
        text_line->error_selected=error_code;
        text_line->show_time=show_time;
      }



}





gboolean gldraw_set_error_codes(GlDrawing *src, int codes){

  int i;
  double tmp1;

  int priority_error=0;

  for(i=1;i<all_errors_count;i++){
    if((codes & (0x01<<(i-1)))){
       priority_error=i;
       codes = codes ^ (0x01<<(i-1));
       break;
    }
  }

  if((codes & 0x10) == 0x10)gldraw_add_error_code(src, ERROR_CODE_MESSAGE_CAPTION1,-1,-1.0,-1.0);
  if((codes & 0x08) == 0x08)gldraw_add_error_code(src, ERROR_CODE_WARNING_CAPTION2,-1,-1.0,-1.0);
  if((codes & 0x04) == 0x04)gldraw_add_error_code(src, ERROR_CODE_WARNING_CAPTION1,-1,-1.0,-1.0);
  if((codes & 0x02) == 0x02)gldraw_add_error_code(src, ERROR_CODE_ERROR_CAPTION2,-1,-1.0,-1.0);
  if((codes & 0x01) == 0x01)gldraw_add_error_code(src, ERROR_CODE_ERROR_CAPTION1,-1,-1.0,-1.0);

  if(src->big_textline.this_enabled==0){
    set_text_line_flags(src,&src->big_textline,priority_error,1,1,getCurrentTime());
    src->big_textline.error_capture_begin_time=getCurrentTime();
  }
  else{

   if(src->big_textline.error_selected==priority_error){
     set_text_line_flags(src,&src->big_textline,-1,-1,-1,getCurrentTime());
     gldraw_add_error_code(src, src->big_textline.error_selected,1,-1.0,getCurrentTime());
/*
     for(i=0;i<history_errors_full_size;i++){
      if(src->history_textlines[i].error_selected==priority_error &&
         src->history_textlines[i].show_time==1){
        src->history_textlines[i].error_last_time=getCurrentTime();
        break;
      }
     }
*/

     //set_text_line_flags(src,&src->big_textline,priority_error,-1,1,getCurrentTime());
   }
   else {

     tmp1=getCurrentTime();

     if(getCurrentTime()-src->big_textline.error_capture_begin_time > src->time_big_text_capture_time){
       gldraw_add_error_code(src, src->big_textline.error_selected,src->big_textline.show_time,-1.0,getCurrentTime());

       int find=-1;
       for(i=0;i<history_errors_full_size;i++){
         if(src->history_textlines[i].this_enabled==1 &&
            src->history_textlines[i].show_time==1 &&
            src->history_textlines[i].error_selected==priority_error){
              find=i;
              break;
            }
       }

       set_text_line_flags(src,&src->big_textline,priority_error,1,1,getCurrentTime());
       if(find!=-1)src->big_textline.error_create_time=src->history_textlines[find].error_create_time;
       src->big_textline.error_capture_begin_time=getCurrentTime();

     }
     else {
       gldraw_add_error_code(src, priority_error, -1,-1.0,getCurrentTime());
     }


   }

  }

  return 1;

}


int calc_cut_string_len_in_chars_num(struct nk_font *font_ptr,char *str,int max_pixels_str_len){

  if(font_ptr==NULL)return 0;

  int str_len,str_len2;
  int len_pix;
  nk_handle handle1;
  handle1.ptr=font_ptr;

  str_len2=strlen(str);
  str_len=str_len2;

  int i;
  for(i=str_len2;i>0;i--){
    len_pix=nk_font_text_width(handle1,font_ptr->info.height,str,str_len);
    if(len_pix<=max_pixels_str_len)break;
    str_len--;
    if(str_len<=0)return 0;
    if(str[str_len-1]&0xC0){
      str_len--;
    }

  }

  if(str_len<=0)return 0;

/*
  //0x0020, 0x00FF,
  if(str_len<=0)return 0;

  if(str[str_len-1]&0xC0){
        str_len--;
  }
*/
  return str_len;

}



float calc_history_one_line_ly(GlDrawing *src){
  return ((float)src->history_textlines_ly)/((float)history_errors_window_size);
}

float calc_bigtextline_ly(GlDrawing *src){
  return 0.25*((float)src->height);
}

float move_history_textlines_shift_y_one_frame(GlDrawing *src){

  if(src->history_textlines_shift_y<0.0)src->history_textlines_shift_y+=calc_history_one_line_ly(src)/src->fps;
  if(src->history_textlines_shift_y>0.0)src->history_textlines_shift_y=0.0;

}

void update_errors_time(GlDrawing *src){

  int i;
  double time_val;

  time_val=getCurrentTime();

  if(time_val-src->big_textline.error_last_time>src->time_delta_continous_error_min){
    src->big_textline.show_time=0;
  }

  for(i=0;i<history_errors_full_size;i++){
    if(time_val-src->history_textlines[i].error_last_time>src->time_delta_continous_error_min){
      src->history_textlines[i].show_time=0;
    }
  }

}

//show_time_flag_override=-1 = no override
//error_create_time_overridee<0.0 = no override
gboolean gldraw_add_error_code(GlDrawing *src, int code,int show_time_flag_override, double error_create_time_override, double error_last_time_override){

  int i;
  int find;
  int free_slots;
  int flag;
  TextRect tr;

  if(code<0 || code>=all_errors_count)return FALSE;

  if(src->gl_drawing_created==0){

    find=-1;
    for(i=0;i<history_errors_full_size;i++){
      if(src->history_textlines[i].error_selected==code &&
         src->history_textlines[i].show_time==1){
        find=i;
        break;
      }
    }

    if(find>=0){
      set_text_line_flags(src,&src->history_textlines[find],code,-1,1,getCurrentTime());
    }
    else{

      for(i=history_errors_full_size-1;i>0;i--){
        src->history_textlines[i]=src->history_textlines[i-1];
      }

      set_text_line_flags(src,&src->history_textlines[0],code,1,1,getCurrentTime());

    }




  }
  else {
/*
    int first_line_lower_window;

    first_line_lower_window=history_errors_window_size+floor((-src->history_textlines_shift_y)/calc_history_one_line_ly(src));

    TextRect tr;

    if(first_line_lower_window>=0 && first_line_lower_window<history_errors_full_size){
      if(src->history_textlines[first_line_lower_window].this_enabled==1 &&
         src->history_textlines[first_line_lower_window].show_time==1){
        tr=src->history_textlines[first_line_lower_window];
        for(i=history_errors_full_size-1;i>0;i--){
          src->history_textlines[i]=src->history_textlines[i-1];
        }
        src->history_textlines[0]=tr;
        if(first_line_lower_window+1<history_errors_full_size)
          src->history_textlines[first_line_lower_window+1].this_enabled=0;
        src->history_textlines_shift_y-=calc_history_one_line_ly(src);
      }
    }
*/

/*
      flag=1;
      while(flag){
        flag=0;
        for(i=0;i<history_errors_full_size-1;i++){
            if(src->history_textlines[i].show_time==0 &&
               src->history_textlines[i+1].show_time==1){
              tr=src->history_textlines[i];
              src->history_textlines[i]=src->history_textlines[i+1];
              src->history_textlines[i+1]=tr;
              flag=1;
            }
        }
      }
*/
    find=-1;
    for(i=0;i<history_errors_full_size;i++){
      if(src->history_textlines[i].error_selected==code &&
         src->history_textlines[i].show_time==1){
        find=i;
        break;
      }
    }

    if(find>=0){
      set_text_line_flags(src,&src->history_textlines[find],code,-1,1,getCurrentTime());
    }
    else{

        free_slots=((history_errors_full_size-history_errors_window_size)*calc_history_one_line_ly(src)+src->history_textlines_shift_y)/calc_history_one_line_ly(src);

        if(free_slots>0){
          for(i=history_errors_full_size-1;i>0;i--){
            src->history_textlines[i]=src->history_textlines[i-1];
          }
          src->history_textlines_shift_y-=calc_history_one_line_ly(src);
        }
        else{
          for(i=history_errors_window_size-1;i>0;i--){
            src->history_textlines[i]=src->history_textlines[i-1];
          }
        }

        if(show_time_flag_override==-1)set_text_line_flags(src,&src->history_textlines[0],code,1,1,getCurrentTime());
        else set_text_line_flags(src,&src->history_textlines[0],code,1,show_time_flag_override,getCurrentTime());

        if(error_create_time_override>=0.0){
            src->history_textlines[0].error_create_time=error_create_time_override;
        }
        if(error_last_time_override>=0.0){
            src->history_textlines[0].error_last_time=error_last_time_override;
        }


      }




  }


  return TRUE;

}






void calc_one_line(TextRect *text_line,
                   float x, float y, float lx, float ly,
                   struct nk_font *font_ptr,
                   float text_x_caption_begin_percent, float text_x_caption_end_percent,
                   float text_x_time_begin_percent, float text_x_time_end_percent,
                   float text_y,
                   float border_thick,
                   int time_text_visible){

     //text_line=&src->small_textlines[i];

      text_line->border_thick=border_thick;
      if(text_line->border_thick<2.0)text_line->border_thick=2.0;

      text_line->x=x+text_line->border_thick/2.0;
      text_line->y=y+text_line->border_thick/2.0;
      text_line->lx=lx-text_line->border_thick/2.0;
      text_line->ly=ly-text_line->border_thick/2.0;

      text_line->border_x=text_line->x;//+text_line->border_thick/2.0;
      text_line->border_y=text_line->y;//+text_line->border_thick/2.0;
      text_line->border_lx=text_line->lx;//-text_line->border_thick;
      text_line->border_ly=text_line->ly;//-text_line->border_thick;

      text_line->border_blink_flag=0;

      text_line->time_text_visible=time_text_visible;

      text_line->background_enabled_flag=1;

      text_line->text_x=text_line->x+text_x_caption_begin_percent*text_line->lx;
      text_line->text_y=text_line->y+text_y-text_line->border_thick/2.0;
      text_line->text_lx=text_x_caption_end_percent*text_line->lx-text_line->x;
      if(font_ptr!=NULL)text_line->text_ly=font_ptr->info.height;

      text_line->selected_font_ptr=font_ptr;

      memcpy(text_line->border_color,
             errors_properties[text_line->error_selected].border_color,
             4*sizeof(float));

      memcpy(text_line->background_color,
             errors_properties[text_line->error_selected].background_color,
             4*sizeof(float));

      memcpy(text_line->text_color,
             errors_properties[text_line->error_selected].text_color,
             4*sizeof(float));

      int len;
      int len_cut;
      int max_text_len_pixels;

      len=strlen(errors_properties[text_line->error_selected].full_text);
      max_text_len_pixels=text_line->text_lx;
      if(len>0 && len<text_line_size){

         len_cut=calc_cut_string_len_in_chars_num(text_line->selected_font_ptr,
                                                  errors_properties[text_line->error_selected].full_text,
                                                  max_text_len_pixels);

         memcpy(text_line->text_line,
                errors_properties[text_line->error_selected].full_text,
                len_cut);

         text_line->text_line[len_cut]=0;
         //text_line->text_line[len_cut+1]=0;

      }

      text_line->text_time_x=text_line->x+text_x_time_begin_percent*text_line->lx;
      text_line->text_time_y=text_line->y+text_y-text_line->border_thick/2.0;
      text_line->text_time_lx=text_x_time_end_percent*text_line->lx-text_line->x;
      if(font_ptr!=NULL)text_line->text_time_ly=font_ptr->info.height;

      double delta_time;
      delta_time=getCurrentTime()-text_line->error_create_time;
      if(delta_time<60.0){//seconds
        sprintf(text_line->text_time,"%ds",(int)(delta_time));
      }else
      if(delta_time<60.0*60.0){//minutes
        sprintf(text_line->text_time,"%dm",(int)(delta_time/60.0));
      }else
      if(delta_time<60.0*60.0*24.0){//hours
        sprintf(text_line->text_time,"%dh",(int)(delta_time/60.0/60.0));
      }else
      if(delta_time<60.0*60.0*30.0){//days
        sprintf(text_line->text_time,"%dd",(int)(delta_time/60.0/60.0/24.0));
      }else{
        sprintf(text_line->text_time,"month");
      }

}




void calc_all_draw_sizes(GlDrawing *src){


  int i;
  float x,y,lx,ly;
  float font_size_ly, font_size_ly_big;
  float enabled_errors_num=0;
  int first_line_lower_window;



  update_errors_time(src);

  first_line_lower_window=history_errors_window_size+floor((-src->history_textlines_shift_y)/calc_history_one_line_ly(src));

  TextRect tr;

  if(first_line_lower_window>=0 && first_line_lower_window<history_errors_full_size){
    if(src->history_textlines[first_line_lower_window].this_enabled==1 &&
       src->history_textlines[first_line_lower_window].show_time==1){
      tr=src->history_textlines[first_line_lower_window];
      for(i=history_errors_full_size-1;i>0;i--){
        src->history_textlines[i]=src->history_textlines[i-1];
      }
      src->history_textlines[0]=tr;
      if(first_line_lower_window+1<history_errors_full_size)
        src->history_textlines[first_line_lower_window+1].this_enabled=0;
      src->history_textlines_shift_y-=calc_history_one_line_ly(src);
    }
  }



  x=0.05*src->width;
  y=src->history_textlines_y;
  lx=src->width-x*2;
  ly=calc_history_one_line_ly(src);

  for(i=0;i<history_errors_full_size;i++){

      calc_one_line(&src->history_textlines[i],
                   x, y, lx, ly,
                   src->font_small_text_line,
                   0.1, 0.78,
                   0.85, 0.9,
                   ly/2-src->font_small_text_line->info.height/2,
                   src->height/150.0,
                   1);


      y=y+ly;

      if(y+src->history_textlines_shift_y>src->history_textlines_y+src->history_textlines_ly)src->history_textlines[i].this_enabled=0;

  }

  x=0.05*src->width;
  lx=src->width-x*2;
  ly=calc_bigtextline_ly(src);
  y=src->history_textlines_y-ly;

  calc_one_line(&src->big_textline,
                   x, y, lx, ly,
                   src->font_big_text_line,
                   0.082, 0.7,
                   0.82, 0.9,
                   ly/2-src->font_big_text_line->info.height/2,
                   src->height/120.0,
                   1);


  calc_one_line(&src->big_flash_rect,
                   0, 0, src->width, src->height,
                   NULL,
                   0, 0,
                   0, 0,
                   0,
                   src->height/90.0,
                   0);


}








gboolean gldraw_clear_errors(GlDrawing *src){

  //src->errors_updated_flag=1;

  //for(int i=0;i<all_errors_count;i++){
  //  src->error_enabled_flag[i]=0;
  //}

  //src->last_error_code=ERROR_CODE_ERROR_NONE;
  //src->last_error_begin_time=getCurrentTime()-10.0;

  int i;

  for(i=0;i<history_errors_full_size;i++){
    src->history_textlines[i].this_enabled=0;
    src->history_textlines[i].error_last_time=-1;
    src->history_textlines[i].error_selected=0;//dummy error
  }


  return TRUE;

}




void gldraw_first_init(GlDrawing *src){

  src->gl_drawing_created=0;
  //src->error_codes_at_create=-1;

  src->all_context_nk=NULL;
  src->font_small_text_line=NULL;
  src->font_big_text_line=NULL;

  src->history_textlines_shift_y=0;
  //seconds
  src->time_delta_continous_error_min=4.0;
  src->time_big_text_capture_time=3.0;
  src->time_big_rect_flash_delta=2.0;

  //---

  gldraw_clear_errors(src);


  //---

}




gboolean gldraw_init (GstGLContext * context, GlDrawing *src,
                      int width, int height,
                      float fps,
                      int direction,
                      float bg_color_r,
                      float bg_color_g,
                      float bg_color_b,
                      float bg_color_a)
{






    gldraw_close(context,src);

    src->width=width;
    src->height=height;
    src->fps=fps;

    src->history_textlines_y=0.37*src->height;
    src->history_textlines_ly=0.5*src->height;

    src->history_textlines_shift_y=0;

    src->all_context_nk=malloc(sizeof(struct nk_custom));
    memset(&src->all_context_nk->ogl.cmds,0,sizeof(src->all_context_nk->ogl.cmds));

    int MAX_MEMORY=10000000;


    nk_init_default(&src->all_context_nk->ctx, 0);

    src->all_context_nk->ctx.clip.userdata = nk_handle_ptr(0);

/*
    struct nk_font_config conf = nk_font_config(0);
    conf.range = &nk_font_russian_glyph_ranges[0];

//<<<<<<<<<<? free?
    struct nk_font *src->font_small_text_line = nk_font_atlas_add_from_file(&src->all_context_nk->atlas,
                              "/root/Desktop/distr/03.guis/glsoundbar/glsoundbar/glsoundbar/src/DroidSans.ttf", 16, &conf);
//<<<<<<<<<<? free?
*/

    GLint status;


    struct nk_sdl_device *dev = &src->all_context_nk->ogl;

    GError *error = NULL;

    const GstGLFuncs *gl = context->gl_vtable;

    if(gl->GenVertexArrays){
      gl->GenVertexArrays (1, &src->all_context_nk->ogl.vao);
      gl->BindVertexArray (src->all_context_nk->ogl.vao);
    }


    gl->GenBuffers (1, &src->all_context_nk->ogl.vbo);
    gl->GenBuffers (1, &src->all_context_nk->ogl.ebo);

    gl->BindBuffer (GL_ARRAY_BUFFER, src->all_context_nk->ogl.vbo);
    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->all_context_nk->ogl.ebo);

    src->shader = gst_gl_shader_new_link_with_stages (context, &error,
     gst_glsl_stage_new_with_string (context, GL_VERTEX_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      vertex_shader),
     gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
      GST_GLSL_VERSION_NONE,
      GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
      fragment_shader), NULL);


    src->all_context_nk->ogl.attrib_pos = gst_gl_shader_get_attribute_location (src->shader, "Position");
    src->all_context_nk->ogl.attrib_uv = gst_gl_shader_get_attribute_location (src->shader, "TexCoord");
    src->all_context_nk->ogl.attrib_col = gst_gl_shader_get_attribute_location (src->shader, "Color");

    {
        src->all_context_nk->ogl.vs = sizeof(struct nk_sdl_vertex);
        src->all_context_nk->ogl.vp = offsetof(struct nk_sdl_vertex, position);
        src->all_context_nk->ogl.vt = offsetof(struct nk_sdl_vertex, uv);
        src->all_context_nk->ogl.vc = offsetof(struct nk_sdl_vertex, col);

    }
    //glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //nk_sdl_font_stash_begin(context, src, &src->all_context_nk->atlas);
    nk_font_atlas_init_default(&src->all_context_nk->atlas);
    nk_font_atlas_begin(&src->all_context_nk->atlas);


    const void *image;

    int font_w, font_h;


    struct nk_font_config conf = nk_font_config(0);
    conf.range = &nk_font_russian_glyph_ranges[0];


    int font_size;

    float w,h;

    w=width;
    h=height;


//<<<<<<<<<<? free?

   // if(src->width>src->height){
    font_size=calc_history_one_line_ly(src)/1.4;//*w/h*3.0/4.0;
    if(h*1.2>w)font_size*=w/h*3.0/4.0;
    //}else {
     // font_size=calc_history_one_line_ly(src)/2;
    //}
    if(font_size<5)font_size=5;

    src->font_small_text_line = nk_font_atlas_add_from_file(&src->all_context_nk->atlas,
                              "/root/Desktop/distr/03.guis/nuklear/glsoundbar/glsoundbar/glsoundbar/src/DroidSans.ttf",
                              font_size, &conf);

    font_size=calc_bigtextline_ly(src)/2.0;
    if(h*1.2>w)font_size*=w/h*3.0/4.0;
    if(font_size<9)font_size=9;

    src->font_big_text_line = nk_font_atlas_add_from_file(&src->all_context_nk->atlas,
                              "/root/Desktop/distr/03.guis/nuklear/glsoundbar/glsoundbar/glsoundbar/src/DroidSans.ttf",
                              font_size, &conf);



//<<<<<<<<<<? free?

    image = nk_font_atlas_bake(&src->all_context_nk->atlas, &font_w, &font_h, NK_FONT_ATLAS_RGBA32);
    glGenTextures(1, &src->all_context_nk->ogl.font_tex);
    glBindTexture(GL_TEXTURE_2D, src->all_context_nk->ogl.font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)font_w, (GLsizei)font_h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
    glBindTexture(GL_TEXTURE_2D, 0);

    nk_font_atlas_end(&src->all_context_nk->atlas, nk_handle_id((int)src->all_context_nk->ogl.font_tex), &src->all_context_nk->ogl.null);
    if (src->all_context_nk->atlas.default_font)
        nk_style_set_font(&src->all_context_nk->ctx, &src->all_context_nk->atlas.default_font->handle);


    int len1;
    nk_handle handle1;
    handle1.ptr=src->font_small_text_line;
    len1=nk_font_text_width(handle1,16,"asdfb",5);



//<<<<<<<<<<<<<?
    src->all_context_nk->ctx.style.font = &src->font_small_text_line->handle;
    src->all_context_nk->ctx.stacks.fonts.head = 0;
    //nk_style_set_font(&src->all_context_nk->ctx, &src->font_small_text_line->handle);
//<<<<<<<<<<<<<?

    //nk_buffer_init_fixed(&src->all_context_nk->vbuf, (void *)src->all_context_nk->vertices, (nk_size)MAX_VERTEX_MEMORY);
    //nk_buffer_init_fixed(&src->all_context_nk->ebuf, (void *)src->all_context_nk->elements, (nk_size)MAX_ELEMENT_MEMORY);



/*
     gldraw_set_error_codes(src,0x01|0x02|0x04|0x10);
     gldraw_set_error_codes(src,0x01|0x02);
*/


     src->gl_drawing_created=1;





     calc_all_draw_sizes(src);

     //if(src->error_codes_at_create>=0)
     //gldraw_set_error_codes(src, 0x07);
     //src->error_codes_at_create=-1;





  return TRUE;

}



/*
  int errors_updated_flag;

  int error_enabled_flag[all_errors_count];

  int    last_error_code;
  double last_error_begin_time;

  float small_textlines_x;
  float small_textlines_y;
  float small_textlines_ly;
  float small_textlines_lx;
  float small_textlines_num;

  TextRect big_textline;
  TextRect small_textlines[all_errors_count];
  TextRect big_flash_rect;
*/


/*
typedef struct {

    float x,y,lx,ly;

    float border_thick;
    //RGBA
    float border_color[4];
    int  border_blink_flag;

    float background_color[4];
    int  background_enabled_flag;//paint background or not

    float text_x;
    float text_y;
    float text_ly;
    float text_lx;
    float text_color[4];
    char text_line[text_line_size];

    struct nk_font *selected_font_ptr;

}TextRect


typedef struct {

   float text_color[4];
   float border_color[4];
   //0 = minimal weight - message weight
   int weight;
   int error_msg;
   char *full_text;

}ErrorProperties;


;
*/





gboolean gldraw_render(GstGLContext * context, GlDrawing *src)
{

    const GstGLFuncs *gl = context->gl_vtable;

    GLenum err;

    int i;

/*
    if(src->errors_updated_flag==1){
      calc_all_draw_sizes(context, src);
      src->errors_updated_flag=0;
    }
*/



    calc_all_draw_sizes(src);

    move_history_textlines_shift_y_one_frame(src);

/*
    for(i=0;i<history_errors_full_size;i++){
      calc_one_line()
    }
*/
    nk_buffer_init_default(&src->all_context_nk->ogl.cmds);

    struct nk_style_window window_default_padding;
    struct nk_style_window window_zero_padding;

    window_default_padding=src->all_context_nk->ctx.style.window;
    window_zero_padding=src->all_context_nk->ctx.style.window;

    window_zero_padding.contextual_padding = nk_vec2(0,0);
    window_zero_padding.combo_padding = nk_vec2(0,0);
    window_zero_padding.group_padding = nk_vec2(0,0);
    window_zero_padding.menu_padding = nk_vec2(0,0);
    window_zero_padding.min_row_height_padding = 0;
    window_zero_padding.padding = nk_vec2(0,0);
    window_zero_padding.popup_padding = nk_vec2(0,0);
    window_zero_padding.tooltip_padding = nk_vec2(0,0);
    window_zero_padding.header.label_padding = nk_vec2(0,0);
    window_zero_padding.header.padding = nk_vec2(0,0);
    window_zero_padding.spacing = nk_vec2(0,0);
    window_zero_padding.combo_border = 0;
    window_zero_padding.border = 0;
    window_zero_padding.contextual_border = 0;
    window_zero_padding.group_border = 0;
    window_zero_padding.menu_border = 0;
    window_zero_padding.popup_border = 0;
    window_zero_padding.tooltip_border = 0;
    window_zero_padding.scrollbar_size = nk_vec2(0,0);

    src->all_context_nk->ctx.style.window=window_zero_padding;

    struct nk_rect bounds;

    src->all_context_nk->ctx.style.window.background=nk_rgba(0,0,0,0);
    src->all_context_nk->ctx.style.window.fixed_background.data.color=nk_rgba(0,0,0,0);

    //nk_style_push_color(&src->all_context_nk->ctx,&src->all_context_nk->ctx.style.window.background,nk_rgba(0,0,0,0));


    //nk_draw_list_path_rect_to
    //nk_draw_list_path_arc_to_fast


    if (nk_begin(&src->all_context_nk->ctx, "Demo", nk_rect(0, 0, src->width, src->height),NK_WINDOW_NO_SCROLLBAR))
    {

        char buf[100];

        //nk_layout_row_static(&src->all_context_nk->ctx, 15, 70, 1);sprintf(buf,"b=%d",test1);test1++;nk_button_label(&src->all_context_nk->ctx, buf);
        //nk_layout_row_static(&src->all_context_nk->ctx, 20, 75, 1);sprintf(buf,"b=%d",test1);test1++;nk_button_label(&src->all_context_nk->ctx, buf);
        //nk_layout_row_static(&src->all_context_nk->ctx, 20, 75, 1);sprintf(buf,"Русский",test1);test1++;nk_button_label(&src->all_context_nk->ctx, buf);

        struct nk_command_buffer *commands = nk_window_get_canvas(&src->all_context_nk->ctx);

        TextRect *tr;

        //nk_layout_row_static(&src->all_context_nk->ctx, src->height, src->width, 1);


        nk_fill_rect(commands, nk_rect(0,0,src->width,src->height), 0, nk_rgba(255, 255, 255, 100));


        double cur_time=getCurrentTime();

        int blink_flag;

        blink_flag=0;
        if(cur_time-floor(cur_time/src->time_big_rect_flash_delta)*src->time_big_rect_flash_delta<0.5*src->time_big_rect_flash_delta)blink_flag=1;

        if(cur_time-src->big_textline.error_capture_begin_time > src->time_big_text_capture_time &&
           src->big_textline.show_time==0)blink_flag=1;


        tr=&src->big_flash_rect;
        if(blink_flag==1){
          nk_stroke_rect(commands, nk_rect(tr->border_x+0.0,tr->border_y+0.0,tr->border_lx+0.0,tr->border_ly+0.0), tr->border_thick*3, tr->border_thick,
                         nk_rgba(src->big_textline.border_color[0],
                                 src->big_textline.border_color[1],
                                 src->big_textline.border_color[2],
                                 src->big_textline.border_color[3]));
        }




        nk_layout_row_static(&src->all_context_nk->ctx, src->history_textlines_y, src->width, 1);
        if (nk_group_begin(&src->all_context_nk->ctx, "Group0", NK_WINDOW_NO_SCROLLBAR)) {



           tr=&src->big_textline;
            if(tr->this_enabled==1){
                if(tr->background_enabled_flag==1)
                  nk_fill_rect(commands, nk_rect(tr->x+0.0,tr->y+0.0,tr->lx+0.0,tr->ly+0.0), tr->border_thick*3, nk_rgba(tr->background_color[0],
                                                                                         tr->background_color[1],
                                                                                         tr->background_color[2],
                                                                                         tr->background_color[3]));

                nk_stroke_rect(commands, nk_rect(tr->border_x+0.0,tr->border_y+0.0,tr->border_lx+0.0,tr->border_ly+0.0), tr->border_thick*3, tr->border_thick,
                                 nk_rgba(tr->border_color[0],
                                         tr->border_color[1],
                                         tr->border_color[2],
                                         tr->border_color[3]));

                nk_draw_text(commands, nk_rect(tr->text_x+0.0,tr->text_y+0.0,tr->text_lx+0.0,tr->text_ly+0.0),
                               tr->text_line, strlen(tr->text_line), &src->font_big_text_line->handle,//src->font_small_text_line,
                               nk_rgba(tr->background_color[0],
                                       tr->background_color[1],
                                       tr->background_color[2],
                                       tr->background_color[3]),
                               nk_rgba(tr->text_color[0],
                                       tr->text_color[1],
                                       tr->text_color[2],
                                       tr->text_color[3]));


                if(tr->show_time==1 && tr->time_text_visible==1 &&
                   getCurrentTime()-tr->error_create_time>src->time_delta_continous_error_min){
                     nk_draw_text(commands, nk_rect(tr->text_time_x+0.0,tr->text_time_y+0.0,tr->text_time_lx+0.0,tr->text_time_ly+0.0),
                               tr->text_time, strlen(tr->text_time), &src->font_big_text_line->handle,//src->font_small_text_line,
                               nk_rgba(tr->background_color[0],
                                       tr->background_color[1],
                                       tr->background_color[2],
                                       tr->background_color[3]),
                               nk_rgba(tr->text_color[0],
                                       tr->text_color[1],
                                       tr->text_color[2],
                                       tr->text_color[3]));
                }



            }

           nk_group_end(&src->all_context_nk->ctx);
        }

        nk_layout_row_static(&src->all_context_nk->ctx, src->history_textlines_ly, src->width, 1);


        if (nk_group_begin(&src->all_context_nk->ctx, "Group1", NK_WINDOW_NO_SCROLLBAR)) {




/*
            nk_fill_rect(commands, nk_rect(10.0,src->history_textlines_y+10.0,100.0,1000.0), 10, nk_rgba(0,255,0,255));
            nk_fill_rect(commands, nk_rect(20.0,src->history_textlines_y+20.0,100.0,1000.0), 10, nk_rgba(255,255,0,255));
            nk_fill_rect(commands, nk_rect(40.0,src->history_textlines_y+40.0,100.0,1000.0), 10, nk_rgba(255,255,255,255));
            nk_fill_rect(commands, nk_rect(60.0,src->history_textlines_y+80.0,100.0,1000.0), 10, nk_rgba(255,0,255,255));
            nk_fill_rect(commands, nk_rect(80.0,src->history_textlines_y+120.0,100.0,1000.0), 10, nk_rgba(255,0,0,255));
*/


            for(i=0;i<history_errors_full_size;i++){
              tr=&src->history_textlines[i];
              if(tr->this_enabled==1){
                  if(tr->background_enabled_flag==1)
                    nk_fill_rect(commands, nk_rect(tr->x+0.0,tr->y+0.0+src->history_textlines_shift_y,tr->lx+0.0,tr->ly+0.0), tr->border_thick*3, nk_rgba(tr->background_color[0],
                                                                                         tr->background_color[1],
                                                                                         tr->background_color[2],
                                                                                         tr->background_color[3]));

                  nk_stroke_rect(commands, nk_rect(tr->border_x+0.0,tr->border_y+0.0+src->history_textlines_shift_y,tr->border_lx+0.0,tr->border_ly+0.0), tr->border_thick*3, tr->border_thick,
                                 nk_rgba(tr->border_color[0],
                                         tr->border_color[1],
                                         tr->border_color[2],
                                         tr->border_color[3]));

                  nk_draw_text(commands, nk_rect(tr->text_x+0.0,tr->text_y+0.0+src->history_textlines_shift_y,tr->text_lx+0.0,tr->text_ly+0.0),
                               tr->text_line, strlen(tr->text_line), &src->font_small_text_line->handle,//src->all_context_nk->ctx.style.font,
                               nk_rgba(tr->background_color[0],
                                       tr->background_color[1],
                                       tr->background_color[2],
                                       tr->background_color[3]),
                               nk_rgba(tr->text_color[0],
                                       tr->text_color[1],
                                       tr->text_color[2],
                                       tr->text_color[3]));


                if(tr->show_time==1 && tr->time_text_visible==1 &&
                   getCurrentTime()-tr->error_create_time>src->time_delta_continous_error_min){
                  nk_draw_text(commands, nk_rect(tr->text_time_x+0.0,tr->text_time_y+0.0+src->history_textlines_shift_y,tr->text_time_lx+0.0,tr->text_time_ly+0.0),
                               tr->text_time, strlen(tr->text_time), &src->font_small_text_line->handle,//src->font_small_text_line,
                               nk_rgba(tr->background_color[0],
                                       tr->background_color[1],
                                       tr->background_color[2],
                                       tr->background_color[3]),
                               nk_rgba(tr->text_color[0],
                                       tr->text_color[1],
                                       tr->text_color[2],
                                       tr->text_color[3]));
                }


              }

            }





          nk_group_end(&src->all_context_nk->ctx);
        }

    }
    nk_end(&src->all_context_nk->ctx);


    struct nk_vec2 scale;

    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };

    ortho[0][0] /= (GLfloat)src->width;
    ortho[1][1] /= (GLfloat)src->height;

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);


    if(gl->GenVertexArrays){
      gl->BindVertexArray (src->all_context_nk->ogl.vao);
    }

    gl->BindBuffer (GL_ARRAY_BUFFER, src->all_context_nk->ogl.vbo);
    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->all_context_nk->ogl.ebo);

    gst_gl_shader_use (src->shader);

    gl->EnableVertexAttribArray (src->all_context_nk->ogl.attrib_pos);
    gl->EnableVertexAttribArray (src->all_context_nk->ogl.attrib_uv);
    gl->EnableVertexAttribArray (src->all_context_nk->ogl.attrib_col);

    gl->VertexAttribPointer (src->all_context_nk->ogl.attrib_pos, 2, GL_FLOAT, GL_FALSE, src->all_context_nk->ogl.vs, (void*)src->all_context_nk->ogl.vp);
    gl->VertexAttribPointer (src->all_context_nk->ogl.attrib_uv, 2, GL_FLOAT, GL_FALSE, src->all_context_nk->ogl.vs, (void*)src->all_context_nk->ogl.vt);
    gl->VertexAttribPointer (src->all_context_nk->ogl.attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, src->all_context_nk->ogl.vs, (void*)src->all_context_nk->ogl.vc);

    gst_gl_shader_set_uniform_1f(src->shader, "height", src->height);
    gst_gl_shader_set_uniform_1i(src->shader, "Texture", 0);
    gst_gl_shader_set_uniform_matrix_4fv(src->shader, "ProjMtx", 1, FALSE, &ortho[0][0]);

    const struct nk_draw_command *cmd;
    const nk_draw_index *offset = NULL;

    {
            // fill convert configuration
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_sdl_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };

            //NK_MEMSET(&config, 0, sizeof(config));
            memset(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_sdl_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_sdl_vertex);
            config.null = src->all_context_nk->ogl.null;
            config.circle_segment_count = 36;
            config.curve_segment_count = 36;
            config.arc_segment_count = 36;
            config.global_alpha = 1.0f;
            config.shape_AA = NK_ANTI_ALIASING_OFF;
            config.line_AA = NK_ANTI_ALIASING_OFF;

            // setup buffers to load vertices and elements
            //{struct nk_buffer vbuf, ebuf;
            //nk_buffer_init_fixed(&vbuf, (void *)src->all_context_nk->vertices, (nk_size)MAX_VERTEX_MEMORY);
            //nk_buffer_init_fixed(&ebuf, (void *)src->all_context_nk->elements, (nk_size)MAX_ELEMENT_MEMORY);

            nk_buffer_init_fixed(&src->all_context_nk->vbuf, (void *)src->all_context_nk->vertices, (nk_size)MAX_VERTEX_MEMORY);
            nk_buffer_init_fixed(&src->all_context_nk->ebuf, (void *)src->all_context_nk->elements, (nk_size)MAX_ELEMENT_MEMORY);

            //nk_buffer_init_default(&src->all_context_nk->vbuf);
            //nk_buffer_init_default(&src->all_context_nk->ebuf);

            nk_convert(&src->all_context_nk->ctx, &src->all_context_nk->ogl.cmds, &src->all_context_nk->vbuf, &src->all_context_nk->ebuf, &config);

            //nk_buffer_free(&vbuf);
            //nk_buffer_free(&ebuf);

            int a;
            a=0;

            //}

    }


    glBufferData(GL_ARRAY_BUFFER, (size_t)src->all_context_nk->ctx.draw_list.vertices->memory.size,
                    src->all_context_nk->ctx.draw_list.vertices->memory.ptr, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (size_t)src->all_context_nk->ctx.draw_list.elements->memory.size,
                   src->all_context_nk->ctx.draw_list.elements->memory.ptr, GL_STATIC_DRAW);

    scale.x = 1.0;//(float)display_width/(float)width;
    scale.y = 1.0;//(float)display_height/(float)height;

    // iterate over and execute each draw command
    nk_draw_foreach(cmd, &src->all_context_nk->ctx, &src->all_context_nk->ogl.cmds) {
        if (!cmd->elem_count) continue;
        glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
        err=glGetError();

        GLint x,y,lx,ly;
        x=(GLint)((cmd->clip_rect.x-1));
        y=(GLint)((cmd->clip_rect.y-1));
        lx=(GLint)((cmd->clip_rect.w+1));
        ly=(GLint)((cmd->clip_rect.h+1));

        glScissor(x,y,lx,ly);

        glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
        err=glGetError();
        offset += cmd->elem_count;
    }
    nk_clear(&src->all_context_nk->ctx);

    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
    gl->BindBuffer (GL_ARRAY_BUFFER, 0);
    gl->DisableVertexAttribArray (src->all_context_nk->ogl.attrib_pos);
    gl->BindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    gst_gl_context_clear_shader(context);

    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);

    nk_buffer_free(&src->all_context_nk->ogl.cmds);
    memset(&src->all_context_nk->ogl.cmds, 0, sizeof(src->all_context_nk->ogl.cmds));

    return TRUE;

}

void gldraw_close (GstGLContext * context, GlDrawing *src)
{

  const GstGLFuncs *gl = context->gl_vtable;


  if(src->all_context_nk!=NULL){

      nk_font_atlas_clear(&src->all_context_nk->atlas);
      nk_free(&src->all_context_nk->ctx);

      struct nk_sdl_device *dev = &src->all_context_nk->ogl;

      glDeleteTextures(1, &src->all_context_nk->ogl.font_tex);
      glDeleteBuffers(1, &src->all_context_nk->ogl.vbo);
      glDeleteBuffers(1, &src->all_context_nk->ogl.ebo);
      gl->DeleteVertexArrays(1, &src->all_context_nk->ogl.vao);
      nk_buffer_free(&src->all_context_nk->ogl.cmds);

      free(src->all_context_nk);
  }
  src->all_context_nk=NULL;


  if(src->shader!=NULL){
      gst_object_unref(src->shader);
      src->shader=NULL;
  }

  src->gl_drawing_created=0;

}


