//----------------------------------------------------------------------------------
// File:        DeferredShadingMSAA\src\shaders/ComplexMask.hlsl
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
#include "Common.hlsli"

//--------------------------------------------------------------------------------------
// Texture and Sampler Variables
//--------------------------------------------------------------------------------------
Texture2DMS<float4, SAMPLE_COUNT> texGBufferMS1 : register(t0);
Texture2DMS<float4, SAMPLE_COUNT> texGBufferMS2 : register(t1);
Texture2D texGBuffer2 : register(t2);
	
SamplerState pointSampler : register( s0 );

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct V2P
{
	float4 pos : SV_POSITION;
	float4 posProj : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
V2P QuadVS(uint id : SV_VertexID)
{
    V2P result;
    result.uv = float2((id << 1) & 2, id & 2);
    result.pos = float4(result.uv * float2(2,-2) + float2(-1,1), 0, 1);
	result.posProj = result.pos;
    return result;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(V2P pixel) : SV_Target
{
	if(UseDiscontinuity == 1)
	{
		float4 gBuf1 = texGBufferMS1.Load(pixel.pos.xy, 0);
		float4 gBuf2 = texGBufferMS2.Load(pixel.pos.xy, 0);
		
		float3 normal = gBuf1.xyz;
		float depth = gBuf1.w;
		float3 albedo = gBuf2.xyz;

		[unroll]
		for(int i = 1; i < SAMPLE_COUNT; i++)
		{
			float4 nextGBuf1 = texGBufferMS1.Load(pixel.pos.xy, i);
			float4 nextGBuf2 = texGBufferMS2.Load(pixel.pos.xy, i);
				
			float3 nextNormal = nextGBuf1.xyz;
			float nextDepth = nextGBuf1.w;
			float3 nextAlbedo = nextGBuf2.xyz;

			[branch]
			if(abs(depth - nextDepth) > 0.1f || abs(dot(abs(normal - nextNormal), float3(1, 1, 1))) > 0.1f || abs(dot(albedo - nextAlbedo, float3(1, 1, 1))) > 0.1f)
			{
				clip(-0.5f);
				return 0;
			}
		}
	}
	else if(UseDiscontinuity == 0)
	{
		if(texGBuffer2.Sample(pointSampler, pixel.uv).a)
		{
			clip(-0.5f);
			return 0;
		}
	}

	return 0;
}