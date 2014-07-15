//----------------------------------------------------------------------------------
// File:        DeinterleavedTexturing\src\SSAO\shaders\src/Blur_Common.hlsl
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

Texture2D<float2> AODepthTexture    : register(t0);
sampler PointClampSampler           : register(s0);
sampler LinearClampSampler          : register(s1);

// Radius of the blur kernel, in full-res pixels
#define KERNEL_RADIUS 3

//----------------------------------------------------------------------------------
struct CenterPixelData
{
    float2 UV;
    float Depth;
    float Sharpness;
};

//----------------------------------------------------------------------------------
float CrossBilateralWeight(float R, float SampleDepth, CenterPixelData Center)
{
    const float BlurSigma = (float)KERNEL_RADIUS * 0.5;
    const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);

    float DeltaZ = (SampleDepth - Center.Depth) * Center.Sharpness;

    return exp2(-R*R*BlurFalloff - DeltaZ*DeltaZ);
}

//-------------------------------------------------------------------------
void ProcessSample(float2 AOZ,
                   float R,
                   CenterPixelData Center,
                   inout float TotalAO,
                   inout float TotalW)
{
    float AO = AOZ.x;
    float Z = AOZ.y;

    float W = CrossBilateralWeight(R, Z, Center);
    TotalAO += W * AO;
    TotalW += W;
}

//-------------------------------------------------------------------------
void ProcessRadius(float2 DeltaUV,
                   CenterPixelData Center,
                   inout float TotalAO,
                   inout float TotalW)
{
    [unroll]
    for (float R = 1; R <= KERNEL_RADIUS; R += 1)
    {
        float2 UV = R * DeltaUV + Center.UV;
        float2 AOZ = AODepthTexture.Sample(PointClampSampler, UV).xy;
        ProcessSample(AOZ, R, Center, TotalAO, TotalW);
    }
}

//-------------------------------------------------------------------------
float ComputeBlur(PostProc_VSOut IN,
                  float2 DeltaUV,
                  out float CenterDepth)
{
    float2 AOZ = AODepthTexture.Sample(PointClampSampler, IN.uv).xy;
    CenterDepth = AOZ.y;

    CenterPixelData Center;
    Center.UV = IN.uv;
    Center.Depth = CenterDepth;
    Center.Sharpness = g_BlurSharpness;

    float TotalAO = AOZ.x;
    float TotalW = 1.0;

    ProcessRadius(DeltaUV, Center, TotalAO, TotalW);
    ProcessRadius(-DeltaUV, Center, TotalAO, TotalW);

    return TotalAO / TotalW;
}
