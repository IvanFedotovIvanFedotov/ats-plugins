/*
 * GStreamer gstgltestsrc
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) 2002,2007 David A. Schleef <ds@schleef.org>
 * Copyright (C) 2008 Julien Isorce <julien.isorce@gmail.com>
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
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
 *
 * GStreamer glsoundbar
 * Copyright (C) 2018 NIITV.
 * Ivan Fedotov<ivanfedotovmail@yandex.ru>
 *
 */

#ifndef __GST_SHADERENV_H__
#define __GST_SHADERENV_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglfuncs.h>

#define MAX_ATTRIBUTES 4

typedef struct _XYZW XYZW;
struct _XYZW
{
  gfloat X, Y, Z, W;
};

/* *INDENT-OFF* */
static const GLfloat positions[] = {
     -1.0,  1.0,  0.0, 1.0,
      1.0,  1.0,  0.0, 1.0,
      1.0, -1.0,  0.0, 1.0,
     -1.0, -1.0,  0.0, 1.0,
};

static const GLushort indices_quad[] = { 0, 1, 2, 0, 2, 3 };
/* *INDENT-ON* */

struct attribute
{
  const gchar *name;
  gint location;
  guint n_elements;
  GLenum element_type;
  guint offset;                 /* in bytes */
  guint stride;                 /* in bytes */
};

//Shader environment
struct ShaderEnv
{

  GstGLShader *shader;
  guint vao;
  guint vbo;
  guint vbo_indices;

  //Передача в вершинный шедер массива занчений
  //В данном случае массива вершин и массива цветов.
  //Оба типа передаются как части массива вершин и цветов vertices_with_colors.
  //Соответственно шейдер должен иметь секции на входе:
  //"attribute vec4 position;\n"
  //"attribute vec4 a_color;\n"
  //
  //разрешаем передачу в шейдер вершин из массива vertices_and_colors.x.y.z.w в position
  gboolean enable_shader_using_attribute__position;
  //разрешаем передачу в шейдер цветов из массива vertices_and_colors.r.g.b.a в a_color
  gboolean enable_shader_using_attribute__a_color;
  //Настройка расположения атрибутов в vertices_with_colors для отделения вершин от цыетов
  struct attribute attribute_vertex;

  gint attributes_location;


};
gboolean shader_env_create(GstGLContext *context, struct ShaderEnv *src,
                           const gchar *vertex_shader_src, const gchar *fragment_shader_src);
gboolean shader_env_bind(GstGLContext *context, struct ShaderEnv *src);
gboolean shader_env_data_set(GstGLContext *context, struct ShaderEnv *src);
gboolean shader_env_draw(GstGLContext *context, struct ShaderEnv *src);
gboolean shader_env_unbind(GstGLContext *context, struct ShaderEnv *src);
gboolean shader_env_free(GstGLContext *context, struct ShaderEnv *src);

#endif /* __GST_SHADERENV_H__ */
