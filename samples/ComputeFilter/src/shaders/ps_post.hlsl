//----------------------------------------------------------------------------------
// File:        ComputeFilter\src\shaders/ps_post.hlsl
// SDK Version: v1.2 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////
// Resources

SamplerState sampPointClamp     : register(s0);
SamplerState sampLinearClamp    : register(s1);

Texture2D texScene : register(t0);
Texture2D <float4> texSAT : register(t1);
Texture2D texDepth : register(t2);

////////////////////////////////////////////////////////////////////////////////
// Constant Buffers
cbuffer cbCamera : register( b0 )
{
   row_major matrix c_world_view_proj_xform;
	float3 c_eye_pos;
	float c_z_near;
	float c_z_far;
	float c_dof_near;
	float c_dof_far;
    float c_aperture;
};

////////////////////////////////////////////////////////////////////////////////
// IO Structs

struct PS_INPUT_SCREEN
{
    float4 Pos : SV_POSITION;
    float2 TC : TEXCOORD0;
};

////////////////////////////////////////////////////////////////////////////////
// Filter the SAT

float decode_depth(float z_value, float z_near, float z_far)
{
	return z_far*z_near/(z_far-z_value*(z_far-z_near));
}

float3 BoxFilter(float2 center, float fWidth, float fHeight)
{
	// UL---->UR
	//  |     |
	//  V     V
	// BL---->BR
	// V_average = (BR - BL - UR + UL) / (Area(UL, BR))

	float2 sat_size;
	texSAT.GetDimensions(sat_size.x, sat_size.y);
    float3 value = float3(0,0,0);
    float2 oMax = min(sat_size*center+float2(fWidth, fHeight), sat_size);
    float2 oMin = max(sat_size*center-float2(fWidth, fHeight), float2(0,0));
    value += texSAT.Sample(sampLinearClamp, float2(oMax.x, oMax.y)/sat_size).rgb;
    value -= texSAT.Sample(sampLinearClamp, float2(oMin.x, oMax.y)/sat_size).rgb;
    value -= texSAT.Sample(sampLinearClamp, float2(oMax.x, oMin.y)/sat_size).rgb;
    value += texSAT.Sample(sampLinearClamp, float2(oMin.x, oMin.y)/sat_size).rgb;
    value = value / ((oMax.x-oMin.x)*(oMax.y-oMin.y));
    return value;
}

float3 tonemap(float3 C)
{
	// Filmic -- model film properties
	C = max(0, C - 0.004);
	return (C*(6.2*C+0.5))/(C*(6.2*C+1.7)+0.06);
}

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader

float4 main( in PS_INPUT_SCREEN input ) : SV_Target
{
	float3 color = float3(0,0,0);

    float dist = decode_depth(texDepth.Sample(sampPointClamp, input.TC).r, c_z_near, c_z_far);
	float coc = max((c_dof_near-dist)/c_dof_near, (dist-c_dof_far)/c_dof_far) * c_aperture * 2;
	//return float4( saturate(coc / c_aperture).xxx, 1.f);
    if (coc <= 1.f)
    {
        color = texScene.Sample(sampPointClamp, input.TC).rgb;
    }
	else if (coc <= 1.5f)
	{
		// if radius is less than 2 pixels, do 4 bilinear taps
		color = texScene.Sample(sampPointClamp, input.TC).rgb;
		float2 scene_size;
		texScene.GetDimensions(scene_size.x, scene_size.y);
		color += texScene.Sample(sampLinearClamp, input.TC + float2( coc,  coc)/scene_size).rgb;
		color += texScene.Sample(sampLinearClamp, input.TC + float2(-coc,  coc)/scene_size).rgb;
		color += texScene.Sample(sampLinearClamp, input.TC + float2(-coc, -coc)/scene_size).rgb;
		color += texScene.Sample(sampLinearClamp, input.TC + float2( coc, -coc)/scene_size).rgb;
		color = color / 5.f;
	}
    else
    {
		// Three overlapping boxes do a decent job of approximating a gaussian
		//        +---------------+
		//        |               |
		//     +--+---------------+--+
		//     |  |               |  |
		//  +--+--+---------------+--+--+
		//  |  |  |               |  |  |
		//  |  |  |               |  |  |
		//  |  |  |               |  |  |
		//  |  |  |       x       |  |  |
		//  |  |  |               |  |  |
		//  |  |  |               |  |  |
		//  |  |  |               |  |  |
		//  +--+--+---------------+--+--+
		//     |  |               |  |
		//     +--+---------------+--+
		//        |               |
		//        +---------------+

        color += BoxFilter(input.TC, coc, 0.5f*coc);
        color += BoxFilter(input.TC, 0.5f*coc, coc);
        color += BoxFilter(input.TC, 0.75f*coc,  0.75f*coc);
        color = color / 3.f;
    }

    return float4(tonemap(color), 1);
}