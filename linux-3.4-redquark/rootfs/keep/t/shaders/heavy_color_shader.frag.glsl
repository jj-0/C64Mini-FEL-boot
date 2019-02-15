/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2010-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifdef GL_ES
precision mediump float;
#endif

uniform vec4 color;
uniform vec4 zero_input_vec;

void main()
{
	vec4 k = zero_input_vec;
	/* This is purely a dummy shader meant to produce full intensity of the color with a large workload */
	for (int i = 0; i < 1; i++)
	{
		k.x += zero_input_vec.y;
		for (int n = 0; n < 10; n++)
		{
			if (k.x < 0.0) k.x *= -1.0;
			if ( k.x < 1.0 ) k.x = 1.0;
		}
	}
	gl_FragColor = color * k.x;
}

