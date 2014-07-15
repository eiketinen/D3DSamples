//----------------------------------------------------------------------------------
// File:        SoftShadows\media/SoftShadows.fx
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
#include "Common.fxh"
#include "PercentageCloserSoftShadows.fxh"
#include "ContactHardeningShadows.fxh"
#include "Shading.fxh"

//--------------------------------------------------------------------------------------

Geometry_VSOut EyeRender_VS (Geometry_VSIn IN)
{
	Geometry_VSOut OUT;
	float4 WorldPos = mul(IN.Pos, g_world);
	OUT.HPosition = mul(WorldPos, g_viewProj);
	OUT.WorldPos = WorldPos;
	OUT.Normal = IN.Normal;
	OUT.LightPos = mul(WorldPos, g_lightViewProjClip2Tex);
	return OUT;
}

float4 EyeRender_PS (uniform int shadowTechnique, Geometry_VSOut IN) : SV_Target
{
	float2 uv = IN.LightPos.xy / IN.LightPos.w;
	float z = IN.LightPos.z / IN.LightPos.w;

	// Compute gradient using ddx/ddy before any branching
	float2 dz_duv = DepthGradient(uv, z);
	float4 color = Shade(IN.WorldPos, IN.Normal);
	if (IsBlack(color.rgb)) return color;

	// Eye-space z from the light's point of view
	float zEye = mul(IN.WorldPos, g_lightView).z;
    float shadow = 1.0f;
    switch (shadowTechnique)
    {
        case 1:
            shadow = PCSS_Shadow(uv, z, dz_duv, zEye);
            break;

        case 2:
            shadow = PCF_Shadow(uv, z, dz_duv, zEye);
            break;
    }
	return color * shadow;	
}

float4 EyeRenderCHS_PS (Geometry_VSOut IN) : SV_Target
{
	float2 uv = IN.LightPos.xy / IN.LightPos.w;
    float z = IN.LightPos.z / IN.LightPos.w;

    float4 color = Shade(IN.WorldPos, IN.Normal);
    if (IsBlack(color.rgb)) return color;

    float3 tc = float3(uv, z - 0.005f);
    return color * CHS_Shadow(tc);
}

//--------------------------------------------------------------------------------------

struct PostProc_VSOut
{
	float4 pos : SV_Position;
	float2 tex : TEXCOORD;
};

float4 VisTex_PS (PostProc_VSOut IN) : SV_TARGET
{
	float z = g_shadowMap.SampleLevel(PointSampler, IN.tex, 0);
	z = (ZClipToZEye(z) - g_lightZNear) / (g_lightZFar - g_lightZNear);
	return z * 10;
}

//Vertex shader that generates a full screen triangle with texcoords
//To use draw 3 vertices with primitive type triangle
PostProc_VSOut FullScreenTri_VS(uint id : SV_VertexID)
{
	PostProc_VSOut output = (PostProc_VSOut)0.0f;
	output.tex = float2((id << 1) & 2, id & 2);
	output.pos = float4(output.tex * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	return output;
}

Z_VSOut LightRender_VS ( Geometry_VSIn IN )
{
    Z_VSOut OUT;
	float4 WorldPos = mul(IN.Pos, g_world);
    OUT.HPosition = mul(WorldPos, g_lightViewProj);
    return OUT;
}

//--------------------------------------------------------------------------------------

technique11 SoftShadows
{
	pass ShadowMapPass
	{
		SetVertexShader(CompileShader(vs_5_0, LightRender_VS()));
		SetGeometryShader(NULL);
		SetPixelShader(NULL);
		SetRasterizerState(LightRender_RS);
		SetDepthStencilState(ZTestLess_DS, 0x00000000);
		SetBlendState(NoBlending_BS, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xffffffff);
	}
	pass ShadowMapPassCHS
	{
		SetVertexShader(CompileShader(vs_5_0, LightRender_VS()));
		SetGeometryShader(NULL);
		SetPixelShader(NULL);
		SetRasterizerState(LightRenderCHS_RS);
		SetDepthStencilState(ZTestLess_DS, 0x00000000);
		SetBlendState(NoBlending_BS, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xffffffff);
	}
	pass VisTex
	{
		SetVertexShader(CompileShader(vs_5_0, FullScreenTri_VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, VisTex_PS()));
		SetRasterizerState(NoMS_NoCull_RS);
		SetDepthStencilState(NoZTest_DS, 0x00000000);
		SetBlendState(NoBlending_BS, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xffffffff);
	}
	pass ZPrepass
	{
		SetVertexShader(CompileShader(vs_5_0, EyeRender_VS()));
		SetGeometryShader(NULL);
		SetPixelShader(NULL);
		SetRasterizerState(EyeRender_RS);
		SetDepthStencilState(ZTestLess_DS, 0x00000000);
		SetBlendState(NoBlending_BS, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xffffffff);
	}
    pass ZEqualNoShadows
	{
		SetVertexShader(CompileShader(vs_5_0, EyeRender_VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, EyeRender_PS(0)));
		SetRasterizerState(EyeRender_RS);
		SetDepthStencilState(ZTestEqual_DS, 0x00000000);
		SetBlendState(NoBlending_BS, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xffffffff);
	}
	pass ZEqualPCSS
	{
		SetVertexShader(CompileShader(vs_5_0, EyeRender_VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, EyeRender_PS(1)));
		SetRasterizerState(EyeRender_RS);
		SetDepthStencilState(ZTestEqual_DS, 0x00000000);
		SetBlendState(NoBlending_BS, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xffffffff);
	}
	pass CHS
	{
		SetVertexShader(CompileShader(vs_5_0, EyeRender_VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, EyeRenderCHS_PS()));
		SetRasterizerState(EyeRender_RS);
		SetDepthStencilState(ZTestEqual_DS, 0x00000000);
		SetBlendState(NoBlending_BS, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xffffffff);
	}
	pass ZEqualPCF
	{
		SetVertexShader(CompileShader(vs_5_0, EyeRender_VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, EyeRender_PS(2)));
		SetRasterizerState(EyeRender_RS);
		SetDepthStencilState(ZTestEqual_DS, 0x00000000);
		SetBlendState(NoBlending_BS, float4(1.0f, 1.0f, 1.0f, 1.0f), 0xffffffff);
	}
}
