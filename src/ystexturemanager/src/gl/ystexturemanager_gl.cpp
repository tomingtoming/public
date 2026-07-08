/* ////////////////////////////////////////////////////////////

File Name: ystexturemanager_gl.cpp
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

#include "ystexturemanager_gl.h"
#include <ysglheader.h>
#include <string.h>


/* static */ YsTextureManager::ActualTexture *YsTextureManager::Alloc(void)
{
	ActualTexture *texPtr=new ActualTexture;
	glGenTextures(1,&texPtr->texId);
	texPtr->frameBufferId=0;
	return texPtr;
}

/* static */ void YsTextureManager::Delete(YsTextureManager::ActualTexture *texPtr)
{
	glDeleteTextures(1,&texPtr->texId);
	glDeleteFramebuffers(1,&texPtr->frameBufferId);
	delete texPtr;
}

/* static */ YSRESULT YsTextureManager::TransferRGBABitmap(YsTextureManager::ActualTexture *texPtr,int wid,int hei,const unsigned char rgba[])
{
	glBindTexture(GL_TEXTURE_2D,texPtr->texId);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	YsArray <unsigned char> upsideDown;
	upsideDown.Set(wid*hei*4,NULL);
	for(int ys=0; ys<hei; ++ys)
	{
		const int yd=hei-1-ys;
		for(int x=0; x<wid; ++x)
		{
			upsideDown[4*(yd*wid+x)  ]=rgba[4*(ys*wid+x)  ];
			upsideDown[4*(yd*wid+x)+1]=rgba[4*(ys*wid+x)+1];
			upsideDown[4*(yd*wid+x)+2]=rgba[4*(ys*wid+x)+2];
			upsideDown[4*(yd*wid+x)+3]=rgba[4*(ys*wid+x)+3];
		}
	}

	glTexImage2D
	    (GL_TEXTURE_2D,
	     0,
	     GL_RGBA, // I saw in a textbook 4 is for RGBA, but different in OpenGL ES.
	     wid,
	     hei,
	     0,
	     GL_RGBA,
	     GL_UNSIGNED_BYTE,
	     upsideDown);

	return YSOK;
}

/* static */ YSRESULT YsTextureManager::CreateDepthMapRenderTarget(ActualTexture *texPtr,int wid,int hei)
{
	if(0!=texPtr->frameBufferId)
	{
		glDeleteFramebuffers(1,&texPtr->frameBufferId);
	}

	if(0==wid)
	{
		glGetIntegerv(GL_MAX_TEXTURE_SIZE,&wid);
	}
	if(0==hei)
	{
		glGetIntegerv(GL_MAX_TEXTURE_SIZE,&hei);
	}

	glBindTexture(GL_TEXTURE_2D,texPtr->texId);
#if (!defined(GL_ES) || GL_ES==0) && !defined(GL_ES_VERSION_2_0)
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT32,wid,hei,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
#elif defined(__EMSCRIPTEN__)
    // WebGL2 rejects the unsized GL_DEPTH_COMPONENT internalformat that WebGL1
    // (WEBGL_depth_texture) accepted, and WebGL1 conversely rejects the sized
    // GL_DEPTH_COMPONENT24.  The context version is a runtime property here
    // (WebGL1 fallback on SwiftShader), so pick by the actual context.  The
    // value is defined inline because the build uses the GL ES 2.0 headers.
    #ifndef GL_DEPTH_COMPONENT24
    #define GL_DEPTH_COMPONENT24 0x81A6
    #endif
    {
        const char *ver=(const char *)glGetString(GL_VERSION);
        const GLenum internalFormat=(NULL!=ver && NULL!=strstr(ver,"OpenGL ES 3") ? GL_DEPTH_COMPONENT24 : GL_DEPTH_COMPONENT);
        glTexImage2D(GL_TEXTURE_2D,0,internalFormat,wid,hei,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,nullptr);
    }
#else
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,wid,hei,0,GL_DEPTH_COMPONENT,GL_UNSIGNED_INT,nullptr); // ES needs to use GL_UNSIGNED_INT
#endif

#ifdef __EMSCRIPTEN__
	// GLES3/WebGL2: a DEPTH_COMPONENT texture is not filterable; sampling it
	// with LINEAR through a plain sampler2D yields undefined results (0 on
	// ANGLE).  NEAREST is required for the ES 3.00 texture() depth read.
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
#else
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
#endif
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1,&texPtr->frameBufferId);
	glBindFramebuffer(GL_FRAMEBUFFER,texPtr->frameBufferId);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,texPtr->texId,0);
#if (!defined(GL_ES) || GL_ES==0) && !defined(GL_ES_VERSION_2_0)
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE); // <- Without this, frame buffer status becomes 36060, which seems to be GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
#else
//    glGenTextures(1,&colorTexIdent);
//    glBindTexture(GL_TEXTURE_2D,colorTexIdent);
//    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,wid,hei,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);

//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

//    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,colorTexIdent,0);
#endif
    
	printf("Frame Buffer  %d  Texture Id %d\n",texPtr->frameBufferId,texPtr->texId);

	auto sta=glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(sta!=GL_FRAMEBUFFER_COMPLETE)
	{
		switch(sta)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			printf("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
			break;
		//case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		//	printf("GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS\n");
		//	break;
#if (!defined(GL_ES) || GL_ES==0) && !defined(GL_ES_VERSION_2_0)
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			printf("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n");
			break;
#endif
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			printf("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			printf("GL_FRAMEBUFFER_UNSUPPORTED\n");
			break;
		default:
			printf("Unknown error %d\n",sta);
			break;
		}

		printf("Cannot generate a frame buffer.\n");
		return YSERR;
	}

	glBindFramebuffer(GL_FRAMEBUFFER,0);
	return YSOK;
}

/* static */ YSBOOL YsTextureManager::IsTextureReady(YsTextureManager::ActualTexture *texPtr)
{
	if(NULL==texPtr)
	{
		return YSFALSE;
	}
	// Unlike stupid and shameful Direct3D9, OpenGL context stays up until the program deletes it.
	return YSTRUE;
}

static void SelectTexture(int texIdent)
{
	switch(texIdent)
	{
	case 0:
		glActiveTexture(GL_TEXTURE0);
		break;
	case 1:
		glActiveTexture(GL_TEXTURE1);
		break;
	case 2:
		glActiveTexture(GL_TEXTURE2);
		break;
	case 3:
		glActiveTexture(GL_TEXTURE3);
		break;
	case 4:
		glActiveTexture(GL_TEXTURE4);
		break;
	case 5:
		glActiveTexture(GL_TEXTURE5);
		break;
	case 6:
		glActiveTexture(GL_TEXTURE6);
		break;
	case 7:
		glActiveTexture(GL_TEXTURE7);
		break;
	case 8:
		glActiveTexture(GL_TEXTURE8);
		break;
	case 9:
		glActiveTexture(GL_TEXTURE9);
		break;
	}
}

YSRESULT YsTextureManager::Unit::Bind(int texIdent) const
{
	SelectTexture(texIdent);

	YSRESULT res=YSERR;

	if(YSTRUE!=IsActualTextureReady())
	{
		MakeActualTexture();
	}
	if(YSTRUE==IsActualTextureReady())
	{
		glBindTexture(GL_TEXTURE_2D,GetActualTexture()->texId);

	#ifdef GL_TEXTURE_ENV
		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	#endif

		if(FOM_RAW_Z==GetFileType())
		{
			// A depth render target keeps its creation-time sampler state
			// (CLAMP_TO_EDGE, and NEAREST on GLES3/WebGL2 where a depth
			// texture is not filterable: LINEAR would make it incomplete and
			// sample as 0, turning the whole scene into shadow).
		}
		else
		{
			GLint filterType;
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

			if(FILTERTYPE_LINEAR==GetFilterType())
			{
				filterType=GL_LINEAR;
			}
			else
			{
				filterType=GL_NEAREST;
			}
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filterType);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filterType);
		}

	#ifdef GL_TEXTURE_GEN_S
		glDisable(GL_TEXTURE_GEN_S);
	#endif
	#ifdef GL_TEXTURE_GEN_T
		glDisable(GL_TEXTURE_GEN_T);
	#endif

#ifndef __EMSCRIPTEN__
		glEnable(GL_TEXTURE_2D);  // Fixed-function relic; invalid in GLES2.
#endif

		res=YSOK;
	}

	glActiveTexture(GL_TEXTURE0);

	return res;
}

void YsTextureManager::Unbind(int texIdent) const
{
	SelectTexture(texIdent);
	glBindTexture(GL_TEXTURE_2D,0);
}

YSRESULT YsTextureManager::Unit::BindFrameBuffer(void) const
{
	glBindTexture(GL_TEXTURE_2D,0);  // This is needed in MacOSX.
	                                 // It actually makes sense.  Unless the texture is unbound before being written,
	                                 // GPU may still be using it for drawing a fragment, and as a result,
	                                 // drawing to the texture may fail.  It was exactly what was happening, and
	                                 // glClear could clear only a part of the texture unless the texture is unbound.

	glBindFramebuffer(GL_FRAMEBUFFER,texPtr->frameBufferId);
	return YSOK;
}

