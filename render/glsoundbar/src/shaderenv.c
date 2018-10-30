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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>
#include "shaderenv.h"



gboolean shader_env_init_struct_fields(GstGLContext *context, struct ShaderEnv *src){

  src->shader=NULL;
  src->vao=-1;
  src->vbo=-1;
  src->vbo_indices=-1;
  src->attributes_location = -1;

  return TRUE;

}

gboolean shader_env_create(GstGLContext *context, struct ShaderEnv *src,
                           const gchar *vertex_shader_src, const gchar *fragment_shader_src){

  const GstGLFuncs *gl = context->gl_vtable;
  GError *error = NULL;
  shader_env_init_struct_fields(context, src);


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
        vertex_shader_src),
    gst_glsl_stage_new_with_string (context, GL_FRAGMENT_SHADER,
        GST_GLSL_VERSION_NONE,
        GST_GLSL_PROFILE_ES | GST_GLSL_PROFILE_COMPATIBILITY,
        fragment_shader_src), NULL);
  if(!src->shader){
    return FALSE;
  }

  src->attributes_location = gst_gl_shader_get_attribute_location (src->shader, "position");

  gl->BufferData (GL_ARRAY_BUFFER, 4 * sizeof(XYZW), positions, GL_STATIC_DRAW);
  gl->BufferData (GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof (gushort), indices_quad, GL_DYNAMIC_DRAW);

  gl->VertexAttribPointer (src->attributes_location, 4, GL_FLOAT, GL_FALSE, sizeof (GLfloat) * 4, (gpointer) (gintptr) 0);
  gl->EnableVertexAttribArray (src->attributes_location);

  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->BindBuffer (GL_ARRAY_BUFFER, 0);
  gl->BindVertexArray (0);

  return TRUE;

}

gboolean shader_env_bind(GstGLContext *context, struct ShaderEnv *src){

  const GstGLFuncs *gl = context->gl_vtable;

  if(gl->GenVertexArrays){
    gl->BindVertexArray (src->vao);
  }
  gl->BindBuffer (GL_ARRAY_BUFFER, src->vbo);
  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, src->vbo_indices);

  gst_gl_shader_use (src->shader);

  return TRUE;

}

gboolean shader_env_draw(GstGLContext *context, struct ShaderEnv *src){

  const GstGLFuncs *gl = context->gl_vtable;

  gl->DrawElements (GL_TRIANGLES, 6 , GL_UNSIGNED_SHORT,(gpointer) (gintptr) 0);

  return TRUE;

}

gboolean shader_env_unbind(GstGLContext *context, struct ShaderEnv *src){

  const GstGLFuncs *gl = context->gl_vtable;

  gl->BindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
  gl->BindBuffer (GL_ARRAY_BUFFER, 0);

  if(gl->GenVertexArrays){
    gl->BindVertexArray(0);
  }

  gst_gl_context_clear_shader(context);

  gl->DisableVertexAttribArray (src->attributes_location);


  return TRUE;

}

gboolean shader_env_free(GstGLContext *context, struct ShaderEnv *src){

  const GstGLFuncs *gl = context->gl_vtable;

  if (src->shader)
    gst_object_unref (src->shader);

  if (src->vao)
    gl->DeleteVertexArrays (1, &src->vao);

  if (src->vbo)
    gl->DeleteBuffers (1, &src->vbo);

  if (src->vbo_indices)
    gl->DeleteBuffers (1, &src->vbo_indices);

  shader_env_init_struct_fields(context, src);

  return TRUE;

}








