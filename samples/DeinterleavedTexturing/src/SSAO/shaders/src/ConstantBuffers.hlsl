//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO\shaders\src/ConstantBuffers.hlsl
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

#ifndef CONSTANT_BUFFERS_H
#define CONSTANT_BUFFERS_H

#include "SharedDefines.h"

#pragma pack_matrix( row_major )

cbuffer GlobalConstantBuffer : register(b0)
{
    float2 g_InvQuarterResolution;
    float2 g_InvFullResolution;

    float2 g_UVToViewA;
    float2 g_UVToViewB;

    float g_RadiusToScreen;
    float g_R2;
    float g_NegInvR2;
    float g_NDotVBias;

    float g_LinearizeDepthA;
    float g_LinearizeDepthB;
    float g_InverseDepthRangeA;
    float g_InverseDepthRangeB;

    float g_AOMultiplier;
    float g_PowExponent;
    float g_BlurSharpness;
};

cbuffer PerPassConstantBuffer : register(b1)
{
    float4 g_Jitter;
    float2 g_Float2Offset;
};

cbuffer PerSampleConstantBuffer : register(b2)
{
    int g_SampleIndex;
};

#endif //CONSTANT_BUFFERS_H
