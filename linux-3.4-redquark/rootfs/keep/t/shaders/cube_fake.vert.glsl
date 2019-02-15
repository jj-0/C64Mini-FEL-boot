/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2011-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

attribute highp vec4      inVert; 
attribute mediump vec2    inUV0;
attribute mediump vec2    inUV1;

uniform mediump vec3    inEyePos;
uniform highp mat4      inVMat; //projected matrix

varying mediump vec4    outUV0;

void main(void)
{
    gl_Position = inVMat * inVert;

    outUV0.xy = inUV0 - inEyePos.xz;
    outUV0.zw = inUV1;
}
