//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\assets\shaders/vs_scene.hlsl
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

#include "constants.hlsl"

////////////////////////////////////////////////////////////////////////////////
// IO Structures

struct VS_INPUT
{
	float3 Position : POSITION0;
	float3 Normal   : NORMAL0;
	float2 TexCoord : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 P    : SV_POSITION;
	float4 PNew : TEXCOORD0;
	float4 POld : TEXCOORD1;
	float2 TC   : TEXCOORD2;
	float  L    : TEXCOORD3;
};

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader

static const float3 light_pos = float3(1.00f, 1.00f, -1.00f);
static const float  light_ambient   =  0.1f;
static const float  light_diffuse   =  0.7f;
static const float  light_specular  =  1.0f;
static const float  light_shininess = 64.0f;

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output;
	float3 PNew_Local = mul(float4(input.Position, 1), c_model_xform_new).xyz;
	float3 POld_Local = mul(float4(input.Position, 1), c_model_xform_old).xyz;

	float4 PNew_EyeSpace = mul(mul(float4(PNew_Local, 1), c_world_xform_new), c_view_xform_new);
	float4 POld_EyeSpace = mul(mul(float4(POld_Local, 1), c_world_xform_old), c_view_xform_old);

	output.PNew = mul(PNew_EyeSpace, c_projection_xform);
	output.POld = mul(POld_EyeSpace, c_projection_xform);
	output.TC = input.TexCoord.xy;
	output.P = output.PNew;

	float3 normal = mul(float4(input.Normal, 1), c_model_xform_normal_new).xyz;
	float3 light_dir = normalize(light_pos);
	float3 h_vector = normalize(light_dir - normalize(PNew_EyeSpace.xyz));

	output.L = light_ambient;
	float n_dot_l = max(dot(normal, light_dir), 0.0f);
	if (n_dot_l > 0.0)
	{
		output.L += (light_diffuse * n_dot_l);
		float n_dot_h = max(dot(normal, h_vector), 0.0f);
		output.L += (light_specular * pow(n_dot_h, light_shininess));
	}

	return output;
}
