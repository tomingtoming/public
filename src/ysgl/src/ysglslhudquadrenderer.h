/* ////////////////////////////////////////////////////////////

File Name: ysglslhudquadrenderer.h
Copyright (c) 2017 Soji Yamakawa.  All rights reserved.
http://www.ysflight.com

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//////////////////////////////////////////////////////////// */

#ifndef YSOPENGL_HUDQUADRENDERER_IS_INCLUDED
#define YSOPENGL_HUDQUADRENDERER_IS_INCLUDED
/* { */

/*! \file
    A single-pass-stereo (OVR_multiview2) renderer that draws one textured quad
    sampling a two-layer sampler2DArray, one array layer per eye selected by
    gl_ViewID_OVR.  It exists only to composite the VR head-up-display texture
    (rendered off-screen into a two-layer multiview framebuffer, both layers
    identical) onto a cockpit-anchored quad inside the main multiview scene
    pass.  The shader is written literally in GLSL ES 3.00 + OVR_multiview2 and
    is NOT run through the ES2->ES3 rewrite (YsGLSLES3ConvertSourceIfNeeded),
    which would prepend a second #version directive.  A renderer therefore only
    exists on a multiview context: YsGLSLCreateHudQuadRenderer returns NULL
    unless YsGLSLGetCompileNumViews()==2. */

#include "ysgldef.h"

/* Force Visual C++ to type-mismatching error. */
#pragma warning( error : 4028)
#pragma warning( error : 4047)

#ifdef __cplusplus
extern "C" {
#endif

struct YsGLSLHudQuadRenderer;

/*! Creates a HUD-quad renderer.  Returns NULL unless the shared-renderer
    compile mode is stereo (YsGLSLGetCompileNumViews()==2): the ES3/multiview
    shader cannot be compiled on a non-multiview context. */
struct YsGLSLHudQuadRenderer *YsGLSLCreateHudQuadRenderer(void);

/*! Deletes a HUD-quad renderer. */
void YsGLSLDeleteHudQuadRenderer(struct YsGLSLHudQuadRenderer *renderer);

/*! Uploads the two-view projection array (2 x mat4 = 32 floats), the same
    projection[gl_ViewID_OVR] array the shared 3D renderer uses for the scene
    pass. */
void YsGLSLSetHudQuadRendererProjectionStereofv(struct YsGLSLHudQuadRenderer *renderer,const GLfloat mat[32]);

/*! Uploads the world->eye0 modelView matrix (16 floats). */
void YsGLSLSetHudQuadRendererModelViewfv(struct YsGLSLHudQuadRenderer *renderer,const GLfloat mat[16]);

/*! Draws the textured quad.  corner is 4 x vec3 (12 floats) in world space,
    winding: bottom-left, bottom-right, top-right, top-left (matching the 0..1
    texcoords hard-coded alongside).  texArrayName is a GL_TEXTURE_2D_ARRAY
    texture object with two layers; layer gl_ViewID_OVR is sampled per eye.
    The caller owns the GL blend/depth state around this call. */
void YsGLSLRenderHudQuad(struct YsGLSLHudQuadRenderer *renderer,const GLfloat corner[12],GLuint texArrayName);

#ifdef __cplusplus
}
#endif

/* } */
#endif
