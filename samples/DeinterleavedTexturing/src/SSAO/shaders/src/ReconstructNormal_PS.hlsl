//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO\shaders\src/ReconstructNormal_PS.hlsl
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

#include "ConstantBuffers.hlsl"
#include "FullScreenTriangle_VS.hlsl"

Texture2D<float>  FullResDepthTexture       : register(t0);
sampler           PointClampSampler         : register(s0);

//----------------------------------------------------------------------------------
float3 UVToView(float2 UV, float viewDepth)
{
    UV = g_UVToViewA * UV + g_UVToViewB;
    return float3(UV * viewDepth, viewDepth);
}

//----------------------------------------------------------------------------------
float3 FetchFullResViewPos(float2 UV)
{
    float ViewDepth = FullResDepthTexture.SampleLevel(PointClampSampler, UV, 0);
    return UVToView(UV, ViewDepth);
}

//----------------------------------------------------------------------------------
float3 MinDiff(float3 P, float3 Pr, float3 Pl)
{
    float3 V1 = Pr - P;
    float3 V2 = P - Pl;
    return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

//----------------------------------------------------------------------------------
float3 ReconstructNormal(float2 UV, float3 P)
{
    float3 Pr = FetchFullResViewPos(UV + float2(g_InvFullResolution.x, 0));
    float3 Pl = FetchFullResViewPos(UV + float2(-g_InvFullResolution.x, 0));
    float3 Pt = FetchFullResViewPos(UV + float2(0, g_InvFullResolution.y));
    float3 Pb = FetchFullResViewPos(UV + float2(0, -g_InvFullResolution.y));
    return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

//----------------------------------------------------------------------------------
float4 MAIN_FUNCTION(PostProc_VSOut IN) : SV_TARGET
{
    float3 ViewPosition = FetchFullResViewPos(IN.uv);
    float3 ViewNormal = ReconstructNormal(IN.uv, ViewPosition);
    return float4(ViewNormal * 0.5 + 0.5, 0);
}
