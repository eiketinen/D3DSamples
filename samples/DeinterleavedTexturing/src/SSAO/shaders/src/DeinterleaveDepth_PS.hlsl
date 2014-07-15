//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO\shaders\src/DeinterleaveDepth_PS.hlsl
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

#define USE_GATHER4 1

Texture2D<float> DepthTexture   : register(t0);
sampler PointClampSampler       : register(s0);

//----------------------------------------------------------------------------------
struct PSOutputDepthTextures
{
    float Z00 : SV_Target0;
    float Z10 : SV_Target1;
    float Z20 : SV_Target2;
    float Z30 : SV_Target3;
    float Z01 : SV_Target4;
    float Z11 : SV_Target5;
    float Z21 : SV_Target6;
    float Z31 : SV_Target7;
};

#if USE_GATHER4

//----------------------------------------------------------------------------------
PSOutputDepthTextures MAIN_FUNCTION(PostProc_VSOut IN)
{
    PSOutputDepthTextures OUT;

    IN.pos.xy = floor(IN.pos.xy) * 4 + (g_Float2Offset + 0.5);
    IN.uv = IN.pos.xy * g_InvFullResolution;

    // Gather sample ordering: (-,+),(+,+),(+,-),(-,-),
    float4 S0 = DepthTexture.GatherRed(PointClampSampler, IN.uv);
    float4 S1 = DepthTexture.GatherRed(PointClampSampler, IN.uv, int2(2,0));

    OUT.Z00 = S0.w;
    OUT.Z10 = S0.z;
    OUT.Z20 = S1.w;
    OUT.Z30 = S1.z;

    OUT.Z01 = S0.x;
    OUT.Z11 = S0.y;
    OUT.Z21 = S1.x;
    OUT.Z31 = S1.y;

    return OUT;
}

#else

//----------------------------------------------------------------------------------
PSOutputDepthTextures MAIN_FUNCTION(PostProc_VSOut IN)
{
    IN.pos.xy = floor(IN.pos.xy) * 4 + g_Float2Offset;
    IN.uv = IN.pos.xy * g_InvFullResolution;

    PSOutputDepthTextures OUT;

    OUT.Z00 = DepthTexture.Sample(PointClampSampler, IN.uv);
    OUT.Z10 = DepthTexture.Sample(PointClampSampler, IN.uv, int2(1,0));
    OUT.Z20 = DepthTexture.Sample(PointClampSampler, IN.uv, int2(2,0));
    OUT.Z30 = DepthTexture.Sample(PointClampSampler, IN.uv, int2(3,0));

    OUT.Z01 = DepthTexture.Sample(PointClampSampler, IN.uv, int2(0,1));
    OUT.Z11 = DepthTexture.Sample(PointClampSampler, IN.uv, int2(1,1));
    OUT.Z21 = DepthTexture.Sample(PointClampSampler, IN.uv, int2(2,1));
    OUT.Z31 = DepthTexture.Sample(PointClampSampler, IN.uv, int2(3,1));

    return OUT;
}

#endif
