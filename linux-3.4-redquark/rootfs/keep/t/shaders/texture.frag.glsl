/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2001-2002, 2007-2010, 2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifdef GL_ES
precision mediump float;
#endif

varying vec4 texcoord0;
uniform sampler2D diffuse;

void main()
{
	gl_FragColor = texture2D(diffuse, texcoord0.xy);
}
