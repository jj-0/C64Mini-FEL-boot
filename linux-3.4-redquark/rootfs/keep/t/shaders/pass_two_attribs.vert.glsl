/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2001-2002, 2007-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

attribute vec4 position;
attribute vec4 in_tex1;
attribute vec4 in_tex0;

uniform mat4 mvp;

varying vec4 texcoord1;
varying vec4 texcoord0;

void main()
{
	texcoord0 = in_tex0;
	texcoord1 = in_tex1;
	
#ifdef GL_ES
	gl_Position = mvp*position;
#else
	gl_Position = mvp*gl_Vertex;
#endif
}


