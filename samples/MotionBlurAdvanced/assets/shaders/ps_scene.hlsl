//----------------------------------------------------------------------------------
// File:        MotionBlurAdvanced\assets\shaders/ps_scene.hlsl
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
// Resources

Texture2D texDiff : register(t0);

////////////////////////////////////////////////////////////////////////////////
// IO Structures

struct VS_OUTPUT
{
	float4 P    : SV_POSITION;
	float4 PNew : TEXCOORD0;
	float4 POld : TEXCOORD1;
	float2 TC   : TEXCOORD2;
	float  L    : TEXCOORD3;
};

struct PS_OUTPUT
{
	float4 C    : SV_Target0;
	float4 V    : SV_Target1;
};

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader

PS_OUTPUT main(VS_OUTPUT input)
{
	PS_OUTPUT final;

	// Calculate the main scene color
	float3 diffuse_albedo = texDiff.Sample(sampLinearClamp, input.TC).rgb;
	final.C = float4(diffuse_albedo * input.L, 1.0f);

	// Calculate the velocity
	float2 vQX = ((input.PNew.xy / input.PNew.w) - (input.POld.xy / input.POld.w)) * c_half_exposure_x_framerate;
	float fLenQX = length(vQX);

	float fWeight = max(0.5, min(fLenQX, c_K));
	fWeight /= (fLenQX + EPSILON1);
	vQX *= fWeight;
	final.V = float4(writeBiasScale(vQX), 0.5f, 1.0f);

	return final;
}
