//----------------------------------------------------------------------------------
// File:        DeferredContexts11\assets/MultithreadedTests11_VS.hlsl
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
// File: MultithreadedTests11_VS.hlsl
//
// The vertex shader file for the MultithreadedTests11 sample.  
// 
//--------------------------------------------------------------------------------------

// Various debug options

//  NOTE this is defined automatically by the C++, don't uncomment manually
//#define VTF_OBJECT_DATA 0 // use a texture instead of constant buffer for world matrix

#define NO_NORMAL_MAP
//#define NO_DYNAMIC_LIGHTING


//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
cbuffer cbPerScene : register( b1 )
{
	matrix		g_mViewProj	: packoffset( c0 );
};

#if VTF_OBJECT_DATA
Texture2D	        g_txVTF					: register( t0 );
#else
cbuffer cbPerObject : register( b0 )
{
	float4x4		g_mWorld	: packoffset( c0 );
};
#endif

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 vPosition	: POSITION;
	float3 vNormal		: NORMAL;
	float2 vTexcoord	: TEXCOORD0;
	float3 vTangent		: TANGENT;
#if VTF_OBJECT_DATA
	uint2  iVTFCoords	: TEXCOORD1;	
#endif
};

struct VS_OUTPUT
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
	float4 vPosition	: SV_POSITION;
};

// We aliased signed vectors as a unsigned format. 
// Need to recover signed values.  The values 1.0 and 2.0
// are slightly inaccurate here.
float3 R10G10B10A2_UNORM_TO_R32G32B32_FLOAT( in float3 vVec )
{
    vVec *= 2.0f;
    return vVec >= 1.0f ? ( vVec - 2.0f ) : vVec;
}

float4x4 decodeMatrix(float3x4 encodedMatrix)
{
    return float4x4(    float4(encodedMatrix[0].xyz,0),
                        float4(encodedMatrix[1].xyz,0),
                        float4(encodedMatrix[2].xyz,0),
                        float4(encodedMatrix[0].w,encodedMatrix[1].w,encodedMatrix[2].w,1));
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VSMain( VS_INPUT Input )
{
	VS_OUTPUT Output;
	
	matrix mWorld;

#if !VTF_OBJECT_DATA
	mWorld = g_mWorld;

#if ENABLE_VERTEX_COLOR
	Output.vColor = float4(mWorld._14, mWorld._24, mWorld._34, mWorld._44);
	mWorld._14 = 0;
	mWorld._24 = 0;
	mWorld._34 = 0;
	mWorld._44 = 1;
#endif
#else
	float4 row1 = g_txVTF.Load(int3(Input.iVTFCoords,0));
	float4 row2 = g_txVTF.Load(int3(Input.iVTFCoords+int2(1,0),0));
	float4 row3 = g_txVTF.Load(int3(Input.iVTFCoords+int2(2,0),0));
	mWorld = decodeMatrix(float3x4(row1,row2,row3));

#if ENABLE_VERTEX_COLOR
	Output.vColor = g_txVTF.Load(int3(Input.iVTFCoords+int2(3,0),0));
#endif
#endif

	Output.vPosWorld = mul( Input.vPosition,    mWorld );
	Output.vPosition = mul( Output.vPosWorld,   g_mViewProj );
	Output.vNormal   = mul( Input.vNormal,      (float3x3)mWorld );
#ifndef NO_NORMAL_MAP
	Output.vTangent  = mul( Input.vTangent,     (float3x3)mWorld );
#endif
	Output.vTexcoord = Input.vTexcoord;

#if SHADER_VARIATION_IDX == 1
	Output.vNormal.x   += 1.0e-4;
#elif SHADER_VARIATION_IDX == 2
	Output.vNormal.y   += 1.0e-4;
#endif

	return Output;
}

