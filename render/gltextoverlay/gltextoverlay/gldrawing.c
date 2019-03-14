/*

gst-launch-1.0 gltextoverlay set-errors=0x07 ! glimagesink

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

#include <fontconfig/fontconfig.h>

#include <stdio.h>



//gst-launch-1.0 gltextoverlay set_errors=0x05 ! glimagesink




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



void setBGColor(GlDrawing *src, unsigned int color){

  src->bgColor[0]=(float)((color & 0xff000000)>>24)/255.0;
  src->bgColor[1]=(float)((color & 0x00ff0000)>>16)/255.0;
  src->bgColor[2]=(float)((color & 0x0000ff00)>>8)/255.0;
  src->bgColor[3]=(float)((color & 0x000000ff))/255.0;

}

unsigned int getBGColor(GlDrawing *src){

  return (((unsigned int)src->bgColor[0])*255 % 256)<<24 |
         (((unsigned int)src->bgColor[1])*255 % 256)<<16 |
         (((unsigned int)src->bgColor[2])*255 % 256)<<8  |
         (((unsigned int)src->bgColor[3])*255 % 256);

}

void setTextColor(GlDrawing *src, unsigned int color){

  src->textColor[0]=(float)((color & 0xff000000)>>24)/255.0;
  src->textColor[1]=(float)((color & 0x00ff0000)>>16)/255.0;
  src->textColor[2]=(float)((color & 0x0000ff00)>>8)/255.0;
  src->textColor[3]=(float)((color & 0x000000ff))/255.0;

}

unsigned int getTextColor(GlDrawing *src){

  return (((unsigned int)src->textColor[0])*255 % 256)<<24 |
         (((unsigned int)src->textColor[1])*255 % 256)<<16 |
         (((unsigned int)src->textColor[2])*255 % 256)<<8  |
         (((unsigned int)src->textColor[3])*255 % 256);

}


void gldraw_set_text(GlDrawing *src, char *_text){
  if(strlen(_text)>TEXT_MAX_SIZE)return;
  strcpy(src->text,_text);
}

char *gldraw_get_text(GlDrawing *src){
  return src->text;
}

void setTextX(GlDrawing *src, float val){
  src->text_texture_x=val;
}
float getTextX(GlDrawing *src){
  return src->text_texture_x;
}

void setTextY(GlDrawing *src, float val){
  src->text_texture_y=val;
}
float getTextY(GlDrawing *src){
  return src->text_texture_y;
}

void setTextSizeMultiplier(GlDrawing *src, float val){
  src->text_size_multiplier=val;
}
float getTextSizeMultiplier(GlDrawing *src){
  return src->text_size_multiplier;
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
        "   gl_Position = vec4(pos4.x,pos4.y,pos4.z,pos4.w);\n"
        "   return;"
        "}\n";


        static const GLchar *fragment_shader =
        NK_SHADER_VERSION
        //"layout(origin_upper_left) in vec4 gl_FragCoord;"
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
         "#endif\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "uniform float text_x;"
        "uniform float text_y;"
        "uniform float text_lx;"
        "uniform float text_ly;"
        "uniform int draw_text_flag;"
        "uniform vec4 text_color;"
        "uniform vec4 bg_color;"
        "void main(){\n"
        "vec4 color;"
        "vec4 color2;"
        //"   gl_FragColor=vec4(1.0,0.5,0.0,1.0);"
        //"   gl_FragColor = Frag_Color;"
        //"   gl_FragColor = texture2D(Texture, Frag_UV);\n"
        //"   gl_FragColor = (Frag_Color+vec4(1.0,1.0,1.0,1.0)) * (texture2D(Texture, Frag_UV)+vec4(1.0,1.0,1.0,1.0));\n"
        "if(draw_text_flag==0){"
        "    color = texture2D(Texture, Frag_UV);\n"
        "    if(Frag_Color.a==1.0){"
        "      gl_FragColor = vec4(0.0,0.0,0.0,0.0);\n"
        "    }"
        "    else if(color.a>0.5){"
        "      gl_FragColor = text_color;\n"
        "    }else{"
        "      gl_FragColor = vec4(0.0,0.0,0.0,0.0);\n"
        "    }"
        //"    if(!(color.r==1.0 && color.g==1.0 && color.b==1.0)){"
        //"      color2=Frag_Color*color;"
        //"      color.a=0.5;"
        //"      gl_FragColor = color;\n"
        //"    }else{"
        //"      gl_FragColor = Frag_Color;\n"
        //"    }"
        //"   gl_FragColor = (Frag_Color) * texture2D(Texture, Frag_UV);\n"
        "}else{"
        "  if(gl_FragCoord.x>=text_x && gl_FragCoord.x<text_x+text_lx && gl_FragCoord.y>=text_y && gl_FragCoord.y<text_y+text_ly){"
        //"    gl_FragColor = (Frag_Color) * (texture2D(Texture, vec2((gl_FragCoord.x-text_x)/text_lx,(gl_FragCoord.y-text_y)/text_ly)));\n"
        "    color = (texture2D(Texture, vec2((gl_FragCoord.x-text_x)/text_lx,(gl_FragCoord.y-text_y)/text_ly)));\n"
        "    if(color.a>0.0){"
        "      gl_FragColor = text_color;\n"
        "    }else{"
        "      gl_FragColor = Frag_Color;\n"
        "    }"
        //"    gl_FragColor = (texture2D(Texture, vec2((gl_FragCoord.x-text_x)/text_lx,(gl_FragCoord.y-text_y)/text_ly)));\n"
        "  }else{"
        "    gl_FragColor = Frag_Color;\n"
        "  }"
        "}"
        "}\n";






/*
static const GLchar *fragment_shader =
        NK_SHADER_VERSION
        //"layout(origin_upper_left) in vec4 gl_FragCoord;"
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
         "#endif\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "uniform float text_x;"
        "uniform float text_y;"
        "uniform float text_lx;"
        "uniform float text_ly;"
        "uniform int draw_text_flag;"
        "void main(){\n"
        //"   gl_FragColor=vec4(1.0,0.5,0.0,1.0);"
        //"   gl_FragColor = Frag_Color;"
        //"   gl_FragColor = texture2D(Texture, Frag_UV);\n"
        //"   gl_FragColor = (Frag_Color+vec4(1.0,1.0,1.0,1.0)) * (texture2D(Texture, Frag_UV)+vec4(1.0,1.0,1.0,1.0));\n"
        "if(draw_text_flag==0){"
        //"  gl_FragColor = (Frag_Color);"
        " if(texture2D(Texture, Frag_UV).x>0.0){"
        "   gl_FragColor = (Frag_Color) * texture2D(Texture, Frag_UV);\n"
        " }else{"
        "   gl_FragColor = vec4(0.0,0.0,0.0,0.0) ;"//*texture2D(Texture, Frag_UV);\n"
        " }"
        //"  gl_FragColor = (Frag_Color) * (texture2D(Texture, Frag_UV));\n"
        "}else{"
        "  if(gl_FragCoord.x>=text_x && gl_FragCoord.x<text_x+text_lx && gl_FragCoord.y>=text_y && gl_FragCoord.y<text_y+text_ly){"
        "   if(texture2D(Texture, vec2((gl_FragCoord.x-text_x)/text_lx,(gl_FragCoord.y-text_y)/text_ly)).x>0.0){"
        "    gl_FragColor = (Frag_Color) * (texture2D(Texture, vec2((gl_FragCoord.x-text_x)/text_lx,(gl_FragCoord.y-text_y)/text_ly)));\n"
        "   }else{"
        "    gl_FragColor = Frag_Color;"//vec4(0.0,0.0,0.0,0.0) ;"//*texture2D(Texture, Frag_UV);\n"
        "   }"
        //"    gl_FragColor = vec4(0.0,1.0,0.0,1.0);\n"
        "  }else{"
        "    gl_FragColor = Frag_Color;\n"
        "  }"
        "}"
        "}\n";


        "    color = (Frag_Color) * (texture2D(Texture, vec2((gl_FragCoord.x-text_x)/text_lx,(gl_FragCoord.y-text_y)/text_ly)));\n"
        "    if(color.r>0.0 || color.g>0.0 || color.b>0.0){"
        "      gl_FragColor = color;\n"
        "    }else{"
        "      gl_FragColor = Frag_Color;\n"
        "    }"


*/










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


  return str_len;

}



void find_font_file(GlDrawing *src){

  FcConfig* config = FcInitLoadConfigAndFonts();
  FcPattern* pat = FcPatternCreate();
  FcObjectSet* os = FcObjectSetBuild (FC_FAMILY, FC_STYLE, FC_LANG, FC_FILE, (char *) 0);
  FcFontSet* fs = FcFontList(config, pat, os);
  //printf("Total matching fonts: %d\n", fs->nfont);
  FcChar8 *file, *style, *family;
  file=NULL;//(FcChar8 *)src->font_caption;
  style=NULL;
  family=NULL;
  //int k;
  int i;



  for (i=0; fs && i < fs->nfont; ++i) {
     FcPattern* font = fs->fonts[i];

     if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
         FcPatternGetString(font, FC_FAMILY, 0, &family) == FcResultMatch &&
         FcPatternGetString(font, FC_STYLE, 0, &style) == FcResultMatch)
     {
        //printf("Filename: [%s] family: [%s] style: [%s]\n", file, family, style);
        //write(file1,buf,strlen(buf));
        if(strcmp(family,src->font_caption)==0 &&
           strcmp(style,src->font_style)==0){
            strcpy(src->font_full_filename,file);
            FcFontSetDestroy(fs);
            return;
        }

     }
  }
  if (fs) FcFontSetDestroy(fs);

  src->font_full_filename[0]=0;

}


void gldraw_set_font_style(GlDrawing *src, char *_font_style){

  strcpy(src->font_style, _font_style);

}

char *gldraw_get_font_style(GlDrawing *src){

  return src->font_style;

}

void gldraw_set_font_caption(GlDrawing *src, char *_font_caption){

  strcpy(src->font_caption, _font_caption);

}

char *gldraw_get_font_caption(GlDrawing *src){

  return src->font_caption;

}

void gldraw_first_init(GlDrawing *src){

  src->gl_drawing_created=0;

  src->font_full_filename[0]=0;
  gldraw_set_font_caption(src, FONT_CAPTION_DEFAULT);
  gldraw_set_font_style(src, FONT_STYLE_DEFAULT);

  src->all_context_nk=NULL;
  src->font_small_text_line=NULL;

  src->text_fbo_created=0;
  src->text_texture=0;
  src->text_fbo=0;

  src->bgColor[0]=0.5;
  src->bgColor[1]=1.0;
  src->bgColor[2]=0.0;
  src->bgColor[3]=1.0;

  src->textColor[0]=0.7;
  src->textColor[1]=1.0;
  src->textColor[2]=1.0;
  src->textColor[3]=0.0;

  src->text_texture_x=0;
  src->text_texture_y=0;
  src->text_texture_lx=1.0;
  src->text_texture_ly=0.1;

  src->text_size_multiplier=1.0;



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

    find_font_file(src);
    if(src->font_full_filename[0]==0)return FALSE;

    src->width=width;
    src->height=height;
    src->fps=fps;


    src->all_context_nk=malloc(sizeof(struct nk_custom));
    memset(&src->all_context_nk->ogl.cmds,0,sizeof(src->all_context_nk->ogl.cmds));

    nk_init_default(&src->all_context_nk->ctx, 0);

    src->all_context_nk->ctx.clip.userdata = nk_handle_ptr(0);

/*
//<<<<<<<<<<? free?
    struct nk_font *src->font_small_text_line = nk_font_atlas_add_from_file(&src->all_context_nk->atlas,
                              "/root/Desktop/distr/03.guis/glsoundbar/glsoundbar/glsoundbar/src/DroidSans.ttf", 16, &conf);
//<<<<<<<<<<? free?
*/


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

    //w=width;
    //h=height;


//<<<<<<<<<<? free?

   // if(src->width>src->height){
    font_size=floor(src->text_texture_ly*height*src->text_size_multiplier);//*w/h*3.0/4.0;
    if(font_size<5)font_size=5;
    src->font_small_text_line = nk_font_atlas_add_from_file(&src->all_context_nk->atlas,
                              src->font_full_filename,
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

    nk_handle handle1;
    handle1.ptr=src->font_small_text_line;


//<<<<<<<<<<<<<?
    src->all_context_nk->ctx.style.font = &src->font_small_text_line->handle;
    src->all_context_nk->ctx.stacks.fonts.head = 0;
    //nk_style_set_font(&src->all_context_nk->ctx, &src->font_small_text_line->handle);
//<<<<<<<<<<<<<?

     src->gl_drawing_created=1;


     //src->text_fbo_created=1;


     glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &src->gstreamer_draw_fbo);
     //glGetIntegerv(GL_TEXTURE_BINDING_2D, &src->gstreamer_texture);

     glGenFramebuffers(1,&src->text_fbo);
     glGenTextures(1,&src->text_texture);

     glBindFramebuffer(GL_FRAMEBUFFER, src->text_fbo);
     glBindTexture(GL_TEXTURE_2D, src->text_texture);

     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, floor(src->text_texture_lx*src->width), floor(src->text_texture_ly*src->height*src->text_size_multiplier), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
     //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, src->text_texture, 0);
     //glDrawBuffer(GL_COLOR_ATTACHMENT1);

     glViewport(0, 0, floor(src->text_texture_lx*src->width), floor(src->text_texture_ly*src->height*src->text_size_multiplier));

     glClear(GL_COLOR_BUFFER_BIT);


//[[[[[[[[[[[[[[[[[[[[[[[[[[


   GLenum err;

    int i;

    if(src->font_full_filename[0]==0)return FALSE;

    glBindTexture(GL_TEXTURE_2D, src->text_texture);
    glBindTexture(GL_TEXTURE_2D, 0);



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
    src->all_context_nk->ctx.style.window.background=nk_rgba(0,0,0,0);
    src->all_context_nk->ctx.style.window.fixed_background.data.color=nk_rgba(0,0,0,0);

    if (nk_begin(&src->all_context_nk->ctx, "Demo", nk_rect(0, 0, floor(src->text_texture_lx*src->width),
                                                             floor(src->text_texture_ly*src->height*src->text_size_multiplier)),NK_WINDOW_NO_SCROLLBAR))
    {
        struct nk_command_buffer *commands = nk_window_get_canvas(&src->all_context_nk->ctx);
        //nk_fill_rect(commands, nk_rect(0,0,floor(src->text_texture_lx*src->width),floor(src->text_texture_ly*src->height*src->text_size_multiplier)),
        //             0, nk_rgba(src->bgColor[1]*255, src->bgColor[2]*255, src->bgColor[3]*255, src->bgColor[0]*255));
        nk_fill_rect(commands, nk_rect(0,0,floor(src->text_texture_lx*src->width),floor(src->text_texture_ly*src->height*src->text_size_multiplier)),
                     0, nk_rgba(0, 0, 0, 255));
        nk_layout_row_static(&src->all_context_nk->ctx, 0, floor(src->text_texture_lx*src->width), 1);
        if (nk_group_begin(&src->all_context_nk->ctx, "Group0", NK_WINDOW_NO_SCROLLBAR)) {
             nk_draw_text(commands, nk_rect(0.0,0.0,floor(src->text_texture_lx*src->width),floor(src->text_texture_ly*src->height*src->text_size_multiplier)),
                               src->text, strlen(src->text), &src->font_small_text_line->handle, //&src->font_big_text_line->handle,//src->font_small_text_line,
                               nk_rgba(0,0,0,128),
                               //nk_rgba(src->bgColor[1]*255,255/*src->bgColor[2]*255*/,src->bgColor[3]*255,0/*src->bgColor[0]*255*/),
                               nk_rgba(0,0,0,128));
           nk_group_end(&src->all_context_nk->ctx);
        }
    }
    nk_end(&src->all_context_nk->ctx);

    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };

    ortho[0][0] /= floor((GLfloat)src->text_texture_lx*src->width);
    ortho[1][1] /= floor((GLfloat)src->text_texture_ly*src->height*src->text_size_multiplier);

    glDisable(GL_BLEND);
    //glEnable(GL_BLEND);
    //glBlendEquation(GL_FUNC_ADD);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

    gst_gl_shader_set_uniform_1f(src->shader, "height", floor(src->text_texture_ly*src->height*src->text_size_multiplier));
    gst_gl_shader_set_uniform_1i(src->shader, "Texture", 0);
    gst_gl_shader_set_uniform_matrix_4fv(src->shader, "ProjMtx", 1, FALSE, &ortho[0][0]);
    gst_gl_shader_set_uniform_1f(src->shader, "text_x", src->text_texture_x*src->width);
    gst_gl_shader_set_uniform_1f(src->shader, "text_y", src->text_texture_y*src->height);
    gst_gl_shader_set_uniform_1f(src->shader, "text_lx", floor(src->text_texture_lx*src->width));
    gst_gl_shader_set_uniform_1f(src->shader, "text_ly", floor(src->text_texture_ly*src->height*src->text_size_multiplier));
    gst_gl_shader_set_uniform_1i(src->shader, "draw_text_flag", 0);
    gst_gl_shader_set_uniform_4f(src->shader, "text_color",src->textColor[1],src->textColor[2],src->textColor[3],src->textColor[0]);
    gst_gl_shader_set_uniform_4f(src->shader, "bg_color",src->bgColor[1],src->bgColor[2],src->bgColor[3],src->bgColor[0]);

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

            nk_buffer_init_fixed(&src->all_context_nk->vbuf, (void *)src->all_context_nk->vertices, (nk_size)MAX_VERTEX_MEMORY);
            nk_buffer_init_fixed(&src->all_context_nk->ebuf, (void *)src->all_context_nk->elements, (nk_size)MAX_ELEMENT_MEMORY);


            nk_convert(&src->all_context_nk->ctx, &src->all_context_nk->ogl.cmds, &src->all_context_nk->vbuf, &src->all_context_nk->ebuf, &config);


    }




    glBufferData(GL_ARRAY_BUFFER, (size_t)src->all_context_nk->ctx.draw_list.vertices->memory.size,
                    src->all_context_nk->ctx.draw_list.vertices->memory.ptr, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (size_t)src->all_context_nk->ctx.draw_list.elements->memory.size,
                   src->all_context_nk->ctx.draw_list.elements->memory.ptr, GL_STATIC_DRAW);

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

//[[[[[[[[[[[[[[[[[[[[[[[[[[

    glBindFramebuffer(GL_FRAMEBUFFER, src->gstreamer_draw_fbo);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glBindTexture(GL_TEXTURE_2D, src->gstreamer_texture);


  return TRUE;

}







gboolean gldraw_render(GstGLContext * context, GlDrawing *src)
{

    const GstGLFuncs *gl = context->gl_vtable;

    GLenum err;

    int i;

    if(src->font_full_filename[0]==0)return FALSE;

     //[[[[[[[[[[[[[

    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };

    GLfloat positions[32];

    positions[0]=0.0;
    positions[1]=0.0;
    positions[2]=0.0;
    positions[3]=0.0;
    positions[4]=src->bgColor[1];
    positions[5]=src->bgColor[2];
    positions[6]=src->bgColor[3];
    positions[7]=src->bgColor[0];
    positions[8]=src->width;
    positions[9]=0.0;
    positions[10]=1.0;
    positions[11]=0.0;
    positions[12]=src->bgColor[1];
    positions[13]=src->bgColor[2];
    positions[14]=src->bgColor[3];
    positions[15]=src->bgColor[0];
    positions[16]=src->width;
    positions[17]=src->height;
    positions[18]=1.0;
    positions[19]=1.0;
    positions[20]=src->bgColor[1];
    positions[21]=src->bgColor[2];
    positions[22]=src->bgColor[3];
    positions[23]=src->bgColor[0];
    positions[24]=0.0;
    positions[25]=src->height;
    positions[26]=0.0;
    positions[27]=1.0;
    positions[28]=src->bgColor[1];
    positions[29]=src->bgColor[2];
    positions[30]=src->bgColor[3];
    positions[31]=src->bgColor[0];

    GLushort indices_quad[] = { 0, 1, 2, 0, 2, 3 };

     //[[[[[[[[[[[[[

    ortho[0][0] /= (GLfloat)src->width;
    ortho[1][1] /= (GLfloat)src->height;

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    //glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    if(gl->GenVertexArrays){
      gl->BindVertexArray (src->all_context_nk->ogl.vao);
    }

    gl->BindBuffer (GL_ARRAY_BUFFER, src->all_context_nk->ogl.vbo);
    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->all_context_nk->ogl.ebo);

    gst_gl_shader_use (src->shader);

    glEnableVertexAttribArray (src->all_context_nk->ogl.attrib_pos);
    glEnableVertexAttribArray (src->all_context_nk->ogl.attrib_uv);
    glEnableVertexAttribArray (src->all_context_nk->ogl.attrib_col);
    glVertexAttribPointer (src->all_context_nk->ogl.attrib_pos, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)0);
    glVertexAttribPointer (src->all_context_nk->ogl.attrib_uv, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(2*sizeof(float)));
    glVertexAttribPointer (src->all_context_nk->ogl.attrib_col, 4, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void *)(4*sizeof(float)));


    glBindTexture(GL_TEXTURE_2D, src->text_texture);

    gst_gl_shader_set_uniform_1f(src->shader, "height", src->height);
    gst_gl_shader_set_uniform_1i(src->shader, "Texture", 0);
    gst_gl_shader_set_uniform_matrix_4fv(src->shader, "ProjMtx", 1, FALSE, &ortho[0][0]);
    gst_gl_shader_set_uniform_1f(src->shader, "text_x", src->text_texture_x*src->width);
    gst_gl_shader_set_uniform_1f(src->shader, "text_y", src->text_texture_y*src->height);
    gst_gl_shader_set_uniform_1f(src->shader, "text_lx", floor(src->text_texture_lx*src->width));
    gst_gl_shader_set_uniform_1f(src->shader, "text_ly", floor(src->text_texture_ly*src->height*src->text_size_multiplier));
    gst_gl_shader_set_uniform_1i(src->shader, "draw_text_flag", 1);
    gst_gl_shader_set_uniform_4f(src->shader, "text_color",src->textColor[1],src->textColor[2],src->textColor[3],src->textColor[0]);
    gst_gl_shader_set_uniform_4f(src->shader, "bg_color",src->bgColor[1],src->bgColor[2],src->bgColor[3],src->bgColor[0]);



    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof (gushort), indices_quad, GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);


    gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
    gl->BindBuffer (GL_ARRAY_BUFFER, 0);
    gl->DisableVertexAttribArray (src->all_context_nk->ogl.attrib_pos);
    gl->BindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    gst_gl_context_clear_shader(context);

    glDisable(GL_BLEND);

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

      glDeleteFramebuffers(1,&src->text_fbo);
      glDeleteTextures(1,&src->text_texture);

      free(src->all_context_nk);
  }
  src->all_context_nk=NULL;


  if(src->shader!=NULL){
      gst_object_unref(src->shader);
      src->shader=NULL;
  }


  //if(src->text_fbo_created==1){

     //src->text_fbo_created=0;
  //}


  src->gl_drawing_created=0;

}


