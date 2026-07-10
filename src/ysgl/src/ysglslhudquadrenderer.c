/* ////////////////////////////////////////////////////////////

File Name: ysglslhudquadrenderer.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _WIN32
#define GL_GLEXT_PROTOTYPES
#endif

#include "ysglheader.h"
#include "ysgldef.h"
#include "ysglslhudquadrenderer.h"
#include "ysglslutil.h"

/* The surrounding ysgl sources target GLES2/WebGL1 headers even though the
   web runtime is a WebGL2 context, so the ES3 texture-array enum is not
   declared.  Define it locally (same value as GL_TEXTURE_2D_ARRAY in ES3). */
#ifndef GL_TEXTURE_2D_ARRAY
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#endif

/* This renderer's shader is ES 3.00 + OVR_multiview2 written out literally:
   it is only ever used inside the VR multiview scene pass, never on a mono
   path, so it must NOT go through YsGLSLES3ConvertSourceIfNeeded (that helper
   prepends its own "#version 300 es", which would collide with the one below).

   gl_ViewID_OVR is read in the FRAGMENT stage to pick the texture-array layer.
   OVR_multiview2 (unlike the original OVR_multiview) explicitly permits
   view-dependent fragment computation, so a direct read is spec-legal.  If a
   particular WebGL2/ANGLE compiler rejected it, the fallback is to pass the
   view id from the vertex stage as a "flat" integer varying. */
static const char *hudQuadVertexShaderSrc=
	"#version 300 es\n"
	"#extension GL_OVR_multiview2 : require\n"
	"layout(num_views = 2) in;\n"
	"uniform highp mat4 projection[2];\n"
	"uniform highp mat4 modelView;\n"
	"in vec3 vertex;\n"
	"in vec2 texCoord;\n"
	"out vec2 tc;\n"
	"void main(void)\n"
	"{\n"
	"	tc = texCoord;\n"
	"	gl_Position = projection[gl_ViewID_OVR] * modelView * vec4(vertex, 1.0);\n"
	"}\n";

static const char *hudQuadFragmentShaderSrc=
	"#version 300 es\n"
	"#extension GL_OVR_multiview2 : require\n"
	"precision mediump float;\n"
	"uniform mediump sampler2DArray hudTexture;\n"
	"in vec2 tc;\n"
	"out vec4 fragColor;\n"
	"void main(void)\n"
	"{\n"
	"	fragColor = texture(hudTexture, vec3(tc, float(gl_ViewID_OVR)));\n"
	// Alpha gain: the HUD is drawn as ~1-texel lines, and LINEAR minification
	// on the composite quad averages them toward transparent -- boosting the
	// sampled alpha keeps thin symbology readable against bright sky.
	"	fragColor.a = min(fragColor.a*1.8, 1.0);\n"
	"}\n";

struct YsGLSLHudQuadRenderer
{
	GLuint programId;
	GLuint vertexShaderId,fragmentShaderId;

	GLuint uniformProjectionPos;
	GLuint uniformModelViewPos;
	GLuint uniformTexturePos;

	GLuint attribVertexPos;
	GLuint attribTexCoordPos;
};

struct YsGLSLHudQuadRenderer *YsGLSLCreateHudQuadRenderer(void)
{
	struct YsGLSLHudQuadRenderer *renderer;
	const int errMsgLen=1024;
	char errMsg[1024];
	int compileSta=99999,infoLogLength=99999,acquiredErrMsgLen=99999;
	int linkSta=99999;
	GLuint prevProgramId;

	/* The ES3/multiview shader below cannot compile on a non-multiview context.
	   Only build a renderer when the shared renderers are in stereo compile
	   mode. */
	if(2!=YsGLSLGetCompileNumViews())
	{
		return NULL;
	}

	renderer=(struct YsGLSLHudQuadRenderer *)malloc(sizeof(struct YsGLSLHudQuadRenderer));
	if(NULL==renderer)
	{
		return NULL;
	}

	renderer->vertexShaderId=glCreateShader(GL_VERTEX_SHADER);
	renderer->fragmentShaderId=glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(renderer->vertexShaderId,1,&hudQuadVertexShaderSrc,NULL);
	glShaderSource(renderer->fragmentShaderId,1,&hudQuadFragmentShaderSrc,NULL);

	glCompileShader(renderer->vertexShaderId);
	glGetShaderiv(renderer->vertexShaderId,GL_COMPILE_STATUS,&compileSta);
	glGetShaderiv(renderer->vertexShaderId,GL_INFO_LOG_LENGTH,&infoLogLength);
	printf("HudQuad Vertex Compile Status %d Info Log Length %d\n",compileSta,infoLogLength);
	glGetShaderInfoLog(renderer->vertexShaderId,errMsgLen-1,&acquiredErrMsgLen,errMsg);
	printf("Error Message: %s\n",errMsg);

	glCompileShader(renderer->fragmentShaderId);
	glGetShaderiv(renderer->fragmentShaderId,GL_COMPILE_STATUS,&compileSta);
	glGetShaderiv(renderer->fragmentShaderId,GL_INFO_LOG_LENGTH,&infoLogLength);
	printf("HudQuad Fragment Compile Status %d Info Log Length %d\n",compileSta,infoLogLength);
	glGetShaderInfoLog(renderer->fragmentShaderId,errMsgLen-1,&acquiredErrMsgLen,errMsg);
	printf("Error Message: %s\n",errMsg);

	renderer->programId=glCreateProgram();
	glAttachShader(renderer->programId,renderer->vertexShaderId);
	glAttachShader(renderer->programId,renderer->fragmentShaderId);
	glLinkProgram(renderer->programId);
	glGetProgramiv(renderer->programId,GL_LINK_STATUS,&linkSta);
	glGetProgramiv(renderer->programId,GL_INFO_LOG_LENGTH,&infoLogLength);
	printf("HudQuad Link Status %d Info Log Length %d\n",linkSta,infoLogLength);
	glGetProgramInfoLog(renderer->programId,errMsgLen-1,&acquiredErrMsgLen,errMsg);
	printf("Error Message: %s\n",errMsg);

	renderer->uniformProjectionPos=glGetUniformLocation(renderer->programId,"projection");
	renderer->uniformModelViewPos=glGetUniformLocation(renderer->programId,"modelView");
	renderer->uniformTexturePos=glGetUniformLocation(renderer->programId,"hudTexture");

	renderer->attribVertexPos=glGetAttribLocation(renderer->programId,"vertex");
	renderer->attribTexCoordPos=glGetAttribLocation(renderer->programId,"texCoord");

	glGetIntegerv(GL_CURRENT_PROGRAM,(GLint *)&prevProgramId);
	glUseProgram(renderer->programId);
	glUniform1i(renderer->uniformTexturePos,0);
	glUseProgram(prevProgramId);

	return renderer;
}

void YsGLSLDeleteHudQuadRenderer(struct YsGLSLHudQuadRenderer *renderer)
{
	if(NULL!=renderer)
	{
		glDeleteProgram(renderer->programId);
		glDeleteShader(renderer->vertexShaderId);
		glDeleteShader(renderer->fragmentShaderId);
		free(renderer);
	}
}

void YsGLSLSetHudQuadRendererProjectionStereofv(struct YsGLSLHudQuadRenderer *renderer,const GLfloat mat[32])
{
	if(NULL!=renderer)
	{
		GLuint prevProgramId;
		glGetIntegerv(GL_CURRENT_PROGRAM,(GLint *)&prevProgramId);
		glUseProgram(renderer->programId);
		glUniformMatrix4fv(renderer->uniformProjectionPos,2,GL_FALSE,mat);
		glUseProgram(prevProgramId);
	}
}

void YsGLSLSetHudQuadRendererModelViewfv(struct YsGLSLHudQuadRenderer *renderer,const GLfloat mat[16])
{
	if(NULL!=renderer)
	{
		GLuint prevProgramId;
		glGetIntegerv(GL_CURRENT_PROGRAM,(GLint *)&prevProgramId);
		glUseProgram(renderer->programId);
		glUniformMatrix4fv(renderer->uniformModelViewPos,1,GL_FALSE,mat);
		glUseProgram(prevProgramId);
	}
}

void YsGLSLRenderHudQuad(struct YsGLSLHudQuadRenderer *renderer,const GLfloat corner[12],GLuint texArrayName)
{
	GLuint prevProgramId;

	/* Winding matches the header comment: BL, BR, TR, TL, drawn as a fan. */
	static const GLfloat texCoord[8]=
	{
		0.0f,0.0f,
		1.0f,0.0f,
		1.0f,1.0f,
		0.0f,1.0f
	};

	if(NULL==renderer)
	{
		return;
	}

	glGetIntegerv(GL_CURRENT_PROGRAM,(GLint *)&prevProgramId);
	glUseProgram(renderer->programId);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY,texArrayName);
	glUniform1i(renderer->uniformTexturePos,0);

	glEnableVertexAttribArray(renderer->attribVertexPos);
	glEnableVertexAttribArray(renderer->attribTexCoordPos);
	glVertexAttribPointer(renderer->attribVertexPos,3,GL_FLOAT,GL_FALSE,0,corner);
	glVertexAttribPointer(renderer->attribTexCoordPos,2,GL_FLOAT,GL_FALSE,0,texCoord);

	glDrawArrays(GL_TRIANGLE_FAN,0,4);

	glDisableVertexAttribArray(renderer->attribVertexPos);
	glDisableVertexAttribArray(renderer->attribTexCoordPos);

	glBindTexture(GL_TEXTURE_2D_ARRAY,0);
	glUseProgram(prevProgramId);
}
