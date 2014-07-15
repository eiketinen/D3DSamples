//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO\shaders\src/ReinterleaveAO_PS.hlsl
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

Texture2DArray<float> AOTexture     : register(t0);
Texture2D<float> DepthTexture       : register(t1);
sampler PointSampler                : register(s0);

//----------------------------------------------------------------------------------
struct PSOut
{
#if ENABLE_BLUR
    float2 AOZ  : SV_TARGET;
#else
    float4 AO   : SV_TARGET;
#endif
};

//-------------------------------------------------------------------------
PSOut MAIN_FUNCTION(PostProc_VSOut IN)
{
    PSOut OUT;

#if !ENABLE_BLUR
     // Convert the input-viewport IN.pos to AO texel pos
     IN.pos.xy = GetFullResAOTexelPos(IN);
#endif

    int2 FullResPos = int2(IN.pos.xy);
    int2 Offset = FullResPos & 3;
    int SliceId = Offset.y * 4 + Offset.x;
    int2 QuarterResPos = FullResPos >> 2;

#if ENABLE_BLUR
    float AO = AOTexture.Load(int4(QuarterResPos, SliceId, 0));
    float ViewDepth = DepthTexture.Sample(PointSampler, IN.uv);
    OUT.AOZ = float2(AO, ViewDepth);
#else
    float AO = AOTexture.Load(int4(QuarterResPos, SliceId, 0));
    OUT.AO = pow(saturate(AO), g_PowExponent);
#endif

    return OUT;
}
