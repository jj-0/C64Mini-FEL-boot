/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2011-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

varying mediump vec4    outUV0;

uniform sampler2D		 sampler0;
uniform sampler2D		 sampler1;
uniform samplerCube      sampler2;

void main(void)
{
	mediump vec2 tmp = outUV0.zw;
	gl_FragColor = texture2D(sampler0, outUV0.xy) + texture2D(sampler1, tmp) + textureCube(sampler2, outUV0.xyz, 0.25);
}
