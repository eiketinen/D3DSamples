//----------------------------------------------------------------------------------
// File:        ComputeFilter\src\shaders/ps_scene.hlsl
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

SamplerState sampLinearWrap : register(s0);

Texture2D texDiff : register(t0);
Texture2D texSpec : register(t1);
Texture2D texNorm : register(t2);


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
// IO Structures

struct VS_OUTPUT
{
    float4 ScreenP : SV_POSITION;
	float3 P : TEXCOORD0;
    float2 TC : TEXCOORD1;
	float3 N : NORMAL0;
};

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader

float4 main(VS_OUTPUT input) : SV_Target0
{
	float3 out_color = float3(0, 0, 0);
	float out_alpha = 1;

	float3 light_color = float3(0.60f, 0.55f, 0.40f);
	float3 light_dir = float3(0.00f, 0.992278f, 0.124035f);
	float3 ambient_light = float3(0.05f, 0.05f, 0.08f);
	float spec_power = 200.f;

	float3 normal = normalize(input.N);
	float3 view_dir = c_eye_pos - input.P;
	float3 h_vector = normalize(light_dir + view_dir);


	float3 diffuse_albedo = texDiff.Sample(sampLinearWrap, input.TC).rgb;
	//float3 specular_albedo = texSpec.Sample(sampLinearWrap, input.TC).rgb;

	out_color = diffuse_albedo * ambient_light * saturate(0.5f - 0.5f*dot(normal, light_dir));
	out_color += diffuse_albedo * light_color * saturate(dot(normal, light_dir));
	//out_color += light_color * pow(saturate(dot(input.N, h_vector)), spec_power);

	return float4(out_color, out_alpha);
}
