//----------------------------------------------------------------------------------
// File:        FXAA\media/ModifiedJitteredShadowSampling.hlsl
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

//#define USEGATHER4    

#ifndef PATTERN
//#define PATTERN 44
//#define PATTERN 64
//#define PATTERN 38
#define PATTERN 84
//#define PATTERN 48
#endif

#if PATTERN == 44
#define SAMPLES 16
#elif PATTERN == 64
#define SAMPLES 24
#elif PATTERN == 38
#define SAMPLES 24
#elif PATTERN == 84
#define SAMPLES 32
#elif PATTERN == 48
#define SAMPLES 32
#endif

#if PATTERN == 44
	const static float2 g_vOffsets[SAMPLES] = {
		{0.000000f,0.062500f},
		{-0.125000f,0.000000f},
		{0.000000f,-0.187500f},
		{0.250000f,0.000000f},
		{0.000000f,0.312500f},
		{-0.375000f,0.000000f},
		{0.000000f,-0.437500f},
		{0.500000f,0.000000f},
		{0.000000f,0.562500f},
		{-0.625000f,0.000000f},
		{0.000000f,-0.687500f},
		{0.750000f,0.000000f},
		{0.000000f,0.812500f},
		{-0.875000f,0.000000f},
		{0.000000f,-0.937500f},
		{1.000000f,0.000000f},
    };
#elif (PATTERN == 64)
	const static float2 g_vOffsets[SAMPLES] = {
		{0.000000f,0.041667f},
		{-0.083333f,0.000000f},
		{0.000000f,-0.125000f},
		{0.166667f,0.000000f},
		{0.000000f,0.208333f},
		{-0.250000f,0.000000f},
		{0.000000f,-0.291667f},
		{0.333333f,0.000000f},
		{0.000000f,0.375000f},
		{-0.416667f,0.000000f},
		{0.000000f,-0.458333f},
		{0.500000f,0.000000f},
		{0.000000f,0.541667f},
		{-0.583333f,0.000000f},
		{0.000000f,-0.625000f},
		{0.666667f,0.000000f},
		{0.000000f,0.708333f},
		{-0.750000f,0.000000f},
		{0.000000f,-0.791667f},
		{0.833333f,0.000000f},
		{0.000000f,0.875000f},
		{-0.916667f,0.000000f},
		{0.000000f,-0.958333f},
		{1.000000f,0.000000f}
};
#elif (PATTERN == 38)
	const static float2 g_vOffsets[SAMPLES] = {
		{0.029463f,0.029463f},
		{0.000000f,0.083333f},
		{-0.088388f,0.088388f},
		{-0.166667f,0.000000f},
		{-0.147314f,-0.147314f},
		{0.000000f,-0.250000f},
		{0.206239f,-0.206239f},
		{0.333333f,0.000000f},
		{0.265165f,0.265165f},
		{0.000000f,0.416667f},
		{-0.324091f,0.324091f},
		{-0.500000f,0.000000f},
		{-0.383016f,-0.383016f},
		{0.000000f,-0.583333f},
		{0.441942f,-0.441942f},
		{0.666667f,0.000000f},
		{0.500867f,0.500867f},
		{0.000000f,0.750000f},
		{-0.559793f,0.559793f},
		{-0.833333f,0.000000f},
		{-0.618718f,-0.618718f},
		{0.000000f,-0.916667f},
		{0.677644f,-0.677644f},
		{1.000000f,0.000000f}
};
#elif (PATTERN == 84)
	const static float2 g_vOffsets[SAMPLES] = {
		{0.000000f,0.031250f},
		{-0.062500f,0.000000f},
		{0.000000f,-0.093750f},
		{0.125000f,0.000000f},
		{0.000000f,0.156250f},
		{-0.187500f,0.000000f},
		{0.000000f,-0.218750f},
		{0.250000f,0.000000f},
		{0.000000f,0.281250f},
		{-0.312500f,0.000000f},
		{0.000000f,-0.343750f},
		{0.375000f,0.000000f},
		{0.000000f,0.406250f},
		{-0.437500f,0.000000f},
		{0.000000f,-0.468750f},
		{0.500000f,0.000000f},
		{0.000000f,0.531250f},
		{-0.562500f,0.000000f},
		{0.000000f,-0.593750f},
		{0.625000f,0.000000f},
		{0.000000f,0.656250f},
		{-0.687500f,0.000000f},
		{0.000000f,-0.718750f},
		{0.750000f,0.000000f},
		{0.000000f,0.781250f},
		{-0.812500f,0.000000f},
		{0.000000f,-0.843750f},
		{0.875000f,0.000000f},
		{0.000000f,0.906250f},
		{-0.937500f,0.000000f},
		{0.000000f,-0.968750f},
		{1.000000f,0.000000f}
};
#elif (PATTERN == 48)
	const static float2 g_vOffsets[SAMPLES] = {
		{0.022097f,0.022097f},
		{0.000000f,0.062500f},
		{-0.066291f,0.066291f},
		{-0.125000f,0.000000f},
		{-0.110485f,-0.110485f},
		{0.000000f,-0.187500f},
		{0.154680f,-0.154680f},
		{0.250000f,0.000000f},
		{0.198874f,0.198874f},
		{0.000000f,0.312500f},
		{-0.243068f,0.243068f},
		{-0.375000f,0.000000f},
		{-0.287262f,-0.287262f},
		{0.000000f,-0.437500f},
		{0.331456f,-0.331456f},
		{0.500000f,0.000000f},
		{0.375650f,0.375650f},
		{0.000000f,0.562500f},
		{-0.419845f,0.419845f},
		{-0.625000f,0.000000f},
		{-0.464039f,-0.464039f},
		{0.000000f,-0.687500f},
		{0.508233f,-0.508233f},
		{0.750000f,0.000000f},
		{0.552427f,0.552427f},
		{0.000000f,0.812500f},
		{-0.596621f,0.596621f},
		{-0.875000f,0.000000f},
		{-0.640816f,-0.640816f},
		{0.000000f,-0.937500f},
		{0.685010f,-0.685010f},
		{1.000000f,0.000000f}
};
#endif


//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------

cbuffer cbConstants : register( b0 )
{
    matrix  g_mWorldViewProj        : packoffset( c0 );
    matrix  g_mWorldViewProjLight   : packoffset( c4 );
    float3  g_MaterialAmbientColor  : packoffset( c8 );
    float3  g_MaterialDiffuseColor  : packoffset( c9 );
    float3  g_vLightPos             : packoffset( c10 );        // Light position in model space
    float3  g_LightDiffuse          : packoffset( c11 );
    float   g_fInvRandomRotSize     : packoffset( c12.x );
    float   g_fInvShadowMapSize     : packoffset( c12.y );
    float   g_fFilterWidth          : packoffset( c12.z );
};


//-----------------------------------------------------------------------------------------
// Textures and Samplers
//-----------------------------------------------------------------------------------------
Texture2D   g_txDiffuse                     : register( t0 );
Texture2D   g_txShadowMap                   : register( t1 );
Texture2D   g_txRandomRot                   : register( t2 );

SamplerState g_samPointMirror               : register( s0 );
SamplerState g_samLinearWrap                : register( s1 );
SamplerComparisonState g_samPointCmpClamp   : register( s2 );

//--------------------------------------------------------------------------------------
// shader input/output structure
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Position             : POSITION; // vertex position 
    float3 Normal               : NORMAL;   // this normal comes in per-vertex
    float2 TextureUV            : TEXCOORD0;// vertex texture coords 
};

struct VS_OUTPUT
{
    float4 Position             : SV_POSITION; // vertex position 
    float4 PositionLightSpace   : POSITION;    // lightspace vertex position 
    float3 Diffuse              : COLOR0;      // vertex diffuse color (note that COLOR0 is clamped from 0..1)
    float2 TextureUV            : TEXCOORD0;   // vertex texture coords
};

// Vertex shader for shadowmap rendering
float4 ShadowMapVS( VS_INPUT input ) : SV_POSITION
{
    return mul( g_mWorldViewProjLight, input.Position );
}

//--------------------------------------------------------------------------------------
// This shader computes standard transform and lighting
//--------------------------------------------------------------------------------------
VS_OUTPUT RenderSceneVS( VS_INPUT input )
{
    VS_OUTPUT Output;

    // Transform the position from object space to homogeneous projection space
    Output.Position = mul( g_mWorldViewProj, input.Position );
    Output.PositionLightSpace = mul( g_mWorldViewProjLight, input.Position );

    float3 vLightDir = normalize( g_vLightPos - input.Position.xyz );

    // Calc diffuse color    
    Output.Diffuse = g_MaterialDiffuseColor * g_LightDiffuse * max( 0, dot( input.Normal, vLightDir ) );
    
    // Just copy the texture coordinate through
    Output.TextureUV = input.TextureUV; 
    
    return Output;    
}

// Simple jittered shadowmap filter

float ShadowFilter( float2 vScreenPos, float3 vLightPos )
{
    // Fetch per-pixel random basis
    float4 basis = g_txRandomRot.Sample( g_samPointMirror, vScreenPos ) * g_fFilterWidth * g_fInvShadowMapSize;

    float shadow = 0;
    
    [unroll]
    for (int i = 0; i<SAMPLES; i++)
    {
        float2 vOffset = g_vOffsets[i].xx * basis.xz + g_vOffsets[i].yy * basis.yw;
        shadow += 1.f/SAMPLES * g_txShadowMap.SampleCmp( g_samPointCmpClamp, vLightPos.xy + vOffset, vLightPos.z );
    }
    
    return shadow;
}

// Vectorized shadowmap filter, uses GatherCmpRed to fetch and filter 4 values at once

float ShadowFilterGather4( float2 vScreenPos, float3 vLightPos )
{
    // Fetch per-pixel random basis
    float4 basis = g_txRandomRot.Sample( g_samPointMirror, vScreenPos ) * g_fFilterWidth;   // offsets to Gather instructions are in texel space, so we scale basis by filter width here

    float shadow = 0;
    
    [unroll]
    for (int i = 0; i< (SAMPLES / 4); i++)
    {
        // Fetch and filter 4 values at once
        int2 o0 = (int2)(g_vOffsets[i * 4 + 0].xx * basis.xz + g_vOffsets[i * 4 + 0].yy * basis.yw);
        int2 o1 = (int2)(g_vOffsets[i * 4 + 1].xx * basis.xz + g_vOffsets[i * 4 + 1].yy * basis.yw);
        int2 o2 = (int2)(g_vOffsets[i * 4 + 2].xx * basis.xz + g_vOffsets[i * 4 + 2].yy * basis.yw);
        int2 o3 = (int2)(g_vOffsets[i * 4 + 3].xx * basis.xz + g_vOffsets[i * 4 + 3].yy * basis.yw);

        float4 results = g_txShadowMap.GatherCmpRed( g_samPointCmpClamp, vLightPos.xy, vLightPos.z, o0, o1, o2, o3 );

        shadow += 1.0/SAMPLES * (results.x + results.y + results.z + results.w);
    }
    
    return shadow;
}



float4 RenderScenePS( VS_OUTPUT In ) : SV_TARGET
{ 
    // Some of these transforms should be folded into the mWorldViewProjLight matrix really, but math is so cheap in PS these days...
    
    float3 vLightPos = In.PositionLightSpace.xyz / In.PositionLightSpace.w; // project
    vLightPos.xy = vLightPos.xy * 0.5f + 0.5f;
    vLightPos.y = 1.0f - vLightPos.y;
#ifndef USEGATHER4                        
    float shadow = ShadowFilter( In.Position.xy * g_fInvRandomRotSize, vLightPos );
#else
    float shadow = ShadowFilterGather4( In.Position.xy * g_fInvRandomRotSize, vLightPos );
#endif    
    
    float3 albedo = g_txDiffuse.Sample( g_samLinearWrap, In.TextureUV ).rgb;

    return  sqrt(float4(g_MaterialAmbientColor * albedo * 0.5f * float3(0.5, 0.5, 1.0) + In.Diffuse * albedo * float3(1.5, 1.25, 1.0) * shadow, 1));   
}
