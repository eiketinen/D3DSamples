//----------------------------------------------------------------------------------
// File:        SoftShadows\media/Common.fxh
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

#define MAX_LINEAR_DEPTH 1.e30f

//--------------------------------------------------------------------------------------

SamplerState PointSampler
{
    Filter   = MIN_MAG_MIP_POINT;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(MAX_LINEAR_DEPTH, 0, 0, 0);
};

SamplerComparisonState PCF_Sampler
{
    ComparisonFunc = LESS;
    Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(MAX_LINEAR_DEPTH, 0, 0, 0);
};

SamplerState LinearSampler
{
    Filter   = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

RasterizerState NoMS_NoCull_RS
{
    CullMode = None;
    FillMode = Solid;
    MultisampleEnable = FALSE;
    DepthBias = 0.0;
    SlopeScaledDepthBias = 0.0;
};

RasterizerState EyeRender_RS
{
    CullMode = Back;
    FillMode = Solid;
    MultisampleEnable = TRUE;
    DepthBias = 0.0;
    SlopeScaledDepthBias = 0.0;
};

RasterizerState LightRender_RS
{
    CullMode = Back;
    FillMode = Solid;
    MultisampleEnable = FALSE;
    DepthBias = 1.e5;
    SlopeScaledDepthBias = 8.0;
};

DepthStencilState NoZTest_DS
{
    DepthEnable = FALSE;
};

RasterizerState Wireframe_RS
{
    FillMode = Wireframe;
    MultisampleEnable = TRUE;
    DepthBias = 0.0;
    SlopeScaledDepthBias = 0.0;
};

DepthStencilState ZTestLess_DS
{
    DepthEnable = TRUE;
    DepthFunc = LESS;
    DepthWriteMask = ALL;
};

DepthStencilState ZTestEqual_DS
{
    DepthEnable = TRUE;
    DepthFunc = EQUAL;
    DepthWriteMask = ZERO;
};

BlendState NoBlending_BS
{
    BlendEnable[0] = FALSE;
    RenderTargetWriteMask[0] = 0x0F;
};

RasterizerState LightRenderCHS_RS
{
    CullMode = Back;
    FillMode = Solid;
    MultisampleEnable = FALSE;
    DepthBias = 0.0;
    SlopeScaledDepthBias = 0.0;
};

SamplerState PointSamplerCHS
{
    Filter   = MIN_MAG_MIP_POINT;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(1.0, 1.0, 1.0, 1.0);
};

SamplerComparisonState CmpLessEqualSamplerCHS
{
    ComparisonFunc = LESS_EQUAL;
    Filter = COMPARISON_MIN_MAG_MIP_POINT;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(1.0, 1.0, 1.0, 1.0);
};

//--------------------------------------------------------------------------------------

Texture2D<float> g_shadowMap;

float2 g_lightRadiusUV;
float g_lightWidth;
float g_lightZNear;
float g_lightZFar;

float4 g_shadowMapDimensions;

row_major float4x4 g_world;
row_major float4x4 g_viewProj;
row_major float4x4 g_lightView;
row_major float4x4 g_lightViewProj;
row_major float4x4 g_lightViewProjClip2Tex;

//--------------------------------------------------------------------------------------

struct Geometry_VSIn
{
    float4 Pos    : position;
    float3 Normal : normal;
};

struct Geometry_VSOut
{
    float4 HPosition     : SV_Position;
    float4 WorldPos      : texcoord0;
    float4 LightPos      : texcoord1;
    float3 Normal        : normal;
};

struct Z_VSOut
{
    float4 HPosition     : SV_Position;
};

//--------------------------------------------------------------------------------------

bool IsBlack(float3 C)
{
	return (dot(C, C) == 0);
}