//----------------------------------------------------------------------------------
// File:        DeferredContexts11\assets/MultithreadedTests11_PS.hlsl
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
//--------------------------------------------------------------------------------------
// File: MultithreadedTests11_PS.hlsl
//
// The pixel shader file for the MultithreadedTests11 sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Various debug options
//#define NO_DIFFUSE_MAP
#define NO_NORMAL_MAP
//#define NO_AMBIENT
//#define NO_DYNAMIC_LIGHTING
//#define NO_SHADOW_MAP 1

#define SHADOW_DEPTH_BIAS 1.0e-3f

#define NUMLIGHTS 4

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
static const int g_iNumLights = NUMLIGHTS;
static const int g_iNumShadows = 1; // by convention, the first n lights cast shadows

cbuffer cbPerObject : register( b0 )
{
	float4		g_vObjectColor			: packoffset( c0 );
};

cbuffer cbPerLight : register( b1 )
{
    struct LightDataStruct
    {
	    matrix		m_mLightViewProj;
	    float4		m_vLightPos;
	    float4		m_vLightDir;
	    float4		m_vLightColor;
	    float4		m_vFalloffs;    // x = dist end, y = dist range, z = cos angle end, w = cos range
	} g_LightData[g_iNumLights]         : packoffset( c0 );
};

cbuffer cbPerScene : register( b2 )
{
	float4  	g_vAmbientColor			: packoffset( c0 );
	float4		g_vTintColor			: packoffset( c1 );
};

//--------------------------------------------------------------------------------------
// Textures and Samplers
//--------------------------------------------------------------------------------------
Texture2D	        g_txDiffuse                : register( t0 );
Texture2D	        g_txNormal                 : register( t1 );
Texture2D           g_txShadow[g_iNumShadows]  : register( t2 );

SamplerComparisonState g_samPointClamp : register( s0 );
SamplerState           g_samLinearWrap : register( s1 );

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct PS_INPUT
{
	float3 vNormal		: NORMAL;
#ifndef NO_NORMAL_MAP
	float3 vTangent		: TANGENT;
#endif
	float2 vTexcoord	: TEXCOORD0;
	float4 vPosWorld	: TEXCOORD1;
#if ENABLE_VERTEX_COLOR
	float3 vColor		: COLOR0;
#endif
};

//--------------------------------------------------------------------------------------
// Sample normal map, convert to signed, apply tangent-to-world space transform
//--------------------------------------------------------------------------------------
float3 CalcPerPixelNormal( float2 vTexcoord, float3 vVertNormal, float3 vVertTangent )
{
	// Compute tangent frame
	vVertNormal =   normalize( vVertNormal );	
	vVertTangent =  normalize( vVertTangent );	
	float3 vVertBinormal = normalize( cross( vVertTangent, vVertNormal ) );	
	float3x3 mTangentSpaceToWorldSpace = float3x3( vVertTangent, vVertBinormal, vVertNormal ); 
	
	// Compute per-pixel normal
	float3 vBumpNormal = g_txNormal.Sample( g_samLinearWrap, vTexcoord );
	vBumpNormal = 2.0f * vBumpNormal - 1.0f;
	
	return mul( vBumpNormal, mTangentSpaceToWorldSpace );
}

//--------------------------------------------------------------------------------------
// Test how much pixel is in shadow, using 2x2 percentage-closer filtering
//--------------------------------------------------------------------------------------
float4 CalcUnshadowedAmount( int iShadow, float4 vPosWorld )
{
    matrix mLightViewProj = g_LightData[iShadow].m_mLightViewProj;
    Texture2D txShadow =    g_txShadow[iShadow]; 

    // Compute pixel position in light space
    float4 vLightSpacePos = mul( vPosWorld, mLightViewProj ); 
    vLightSpacePos.xyz /= vLightSpacePos.w;
    
    // Translate from surface coords to texture coords
    // Could fold these into the matrix
    float2 vShadowTexCoord = 0.5f * vLightSpacePos + 0.5f;
    vShadowTexCoord.y = 1.0f - vShadowTexCoord.y;
  
    // Depth bias to avoid pixel self-shadowing
    float vLightSpaceDepth = vLightSpacePos.z - SHADOW_DEPTH_BIAS;
    
    return txShadow.SampleCmpLevelZero(g_samPointClamp, vShadowTexCoord, vLightSpaceDepth);
}

//--------------------------------------------------------------------------------------
// Diffuse lighting calculation, with angle and distance falloff
//--------------------------------------------------------------------------------------
float4 CalcLightingColor( int iLight, float3 vPosWorld, float3 vPerPixelNormal )
{
    float3 vLightPos =      g_LightData[iLight].m_vLightPos.xyz; 
    float3 vLightDir =      g_LightData[iLight].m_vLightDir.xyz;
    float4 vLightColor =    g_LightData[iLight].m_vLightColor; 
    float4 vFalloffs =      g_LightData[iLight].m_vFalloffs; 
    
    float3 vLightToPixelUnNormalized = vPosWorld - vLightPos;
    
    // Dist falloff = 0 at vFalloffs.x, 1 at vFalloffs.x - vFalloffs.y
    float fDist = length( vLightToPixelUnNormalized );
    float fDistFalloff = saturate( ( vFalloffs.x - fDist ) / vFalloffs.y );
    
    // Normalize from here on
    float3 vLightToPixelNormalized = vLightToPixelUnNormalized / fDist;
    
    // Angle falloff = 0 at vFalloffs.z, 1 at vFalloffs.z - vFalloffs.w
    float fCosAngle = dot( vLightToPixelNormalized, vLightDir );
    float fAngleFalloff = saturate( ( fCosAngle - vFalloffs.z ) / vFalloffs.w );
    
    // Diffuse contribution
    float fNDotL = saturate( -dot( vLightToPixelNormalized, vPerPixelNormal ) );
    
	return vLightColor * fNDotL * fDistFalloff * fAngleFalloff;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PSMain( PS_INPUT Input ) : SV_TARGET
{
#ifdef NO_DIFFUSE_MAP
	float4 vDiffuse = 0.5f;
#else   // #ifdef NO_DIFFUSE_MAP
	float4 vDiffuse = g_txDiffuse.Sample( g_samLinearWrap, Input.vTexcoord );
#endif  // #ifdef NO_DIFFUSE_MAP #else
	
	// Compute per-pixel normal
#ifdef NO_NORMAL_MAP
	float3 vPerPixelNormal = Input.vNormal;
#else   // #ifdef NO_NORMAL_MAP
	float3 vPerPixelNormal = CalcPerPixelNormal( Input.vTexcoord, Input.vNormal, Input.vTangent );
#endif  // #ifdef NO_NORMAL_MAP #else

    // Compute lighting contribution
#ifdef NO_AMBIENT
	float4 vTotalLightingColor = float4(0.1,0.1,0.1,0.1);
#else   // #ifdef NO_AMBIENT
	float4 vTotalLightingColor = g_vAmbientColor;
#endif  // #ifdef NO_AMBIENT #else


#ifndef NO_DYNAMIC_LIGHTING
	for ( int iLight = 0; iLight < g_iNumLights; ++iLight )
	{
        float4 vLightingColor = CalcLightingColor( iLight, Input.vPosWorld, vPerPixelNormal );
	    vTotalLightingColor += vLightingColor;
	}
#else
	vTotalLightingColor = float4(1,1,1,1);
#endif  // #ifndef NO_DYNAMIC_LIGHTING


#if NO_SHADOW_MAP
#else
	vTotalLightingColor *= CalcUnshadowedAmount(0, Input.vPosWorld );
#endif

	// TODO :  Add per object color to a texture to load from, or add to VTF and interpolate
	//			Or... skip if it really doesn't matter...^^

	float4 result;
#if ENABLE_VERTEX_COLOR
	result = float4(Input.vColor.xyz,1) + vDiffuse * g_vTintColor * vTotalLightingColor;
#else
	result = g_vObjectColor + vDiffuse * g_vTintColor * vTotalLightingColor;
#endif

#if SHADER_VARIATION_IDX == 1
	result = result.gbra * 0.7;
#elif SHADER_VARIATION_IDX == 2
	result = result.brga * 1.2;
#endif
	return result;
}
