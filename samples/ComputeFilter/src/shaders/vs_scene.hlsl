//----------------------------------------------------------------------------------
// File:        ComputeFilter\src\shaders/vs_scene.hlsl
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

cbuffer cbObject : register( b1 )
{
	row_major matrix c_model_xform;
};

////////////////////////////////////////////////////////////////////////////////
// IO Structures

struct VS_INPUT
{
    float3 Pos : POSITION0;
	float3 Norm : NORMAL0;
	float2 TC : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 ScreenP : SV_POSITION;
	float3 P : TEXCOORD0;
    float2 TC : TEXCOORD1;
	float3 N : NORMAL0;
};

////////////////////////////////////////////////////////////////////////////////
// Vertex Shader

VS_OUTPUT main( VS_INPUT input )
{
    VS_OUTPUT output;
	output.P = mul(float4(input.Pos, 1), c_model_xform).xyz;
	output.ScreenP = mul(float4(output.P, 1), c_world_view_proj_xform);
	output.N = input.Norm;
	output.TC = input.TC.xy;
    return output;
}
