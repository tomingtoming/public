#include "YS_GLSL_HEADER.glsl"

uniform sampler2D bmpTexture;
uniform  LOWP  float alphaCutOff;

varying  MIDP  vec2 texCoord_out;

void main()
{
	 LOWP  vec4 texcell=texture2D(bmpTexture,texCoord_out);
	if(alphaCutOff<texcell.a)
	{
		gl_FragColor=texcell;
	}
	else
	{
		discard;
	}
}
